/******************************************************************
//
//
//  Copyright(C) 2019, Institute for Defense Analyses
//  4850 Mark Center Drive, Alexandria, VA; 703-845-2500
//  This material may be reproduced by or for the US Government
//  pursuant to the copyright license under the clauses at DFARS
//  252.227-7013 and 252.227-7014.
// 
//
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of the copyright holder nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
// 
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//  COPYRIGHT HOLDER NOR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
// 
 *****************************************************************/ 

/*! \file ig_selector.upc
 * \brief A Selector implementation of indexgather.
 */

#include <shmem.h>
extern "C" {
#include <spmat.h>
}
#include "selector.h"

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

/*!
 * \brief This routine implements the conveyor variant of indexgather.
 * \param *tgt array of target locations for the gathered values
 * \param *pckindx array of packed indices for the distributed version of the global array of counts.
 * \param l_num_req the length of the pcindx array
 * \param *ltable localized pointer to the count array.
 * \return average run time
 *
 */

struct IgPkt {
    int64_t idx;
    int64_t val;
};

enum MailBoxType{REQUEST, RESPONSE};

class IgSelector: public hclib::Selector<2, IgPkt> {
  
  //shared table array src, target
  int64_t * ltable, *tgt;

  void req_process(IgPkt pkt, int sender_rank) {
      pkt.val = ltable[pkt.val];
      send(RESPONSE, pkt, sender_rank);
  }

  void resp_process(IgPkt pkt, int sender_rank) {
      tgt[pkt.idx] = pkt.val;
  }

  public:

    IgSelector(int64_t *ltable, int64_t *tgt) : ltable(ltable), tgt(tgt){
        mb[REQUEST].process = [this](IgPkt pkt, int sender_rank) { this->req_process(pkt, sender_rank); };
        mb[RESPONSE].process = [this](IgPkt pkt, int sender_rank) { this->resp_process(pkt, sender_rank); };
    }

};

double ig_selector(int64_t *tgt, int64_t *pckindx, int64_t l_num_req,  int64_t *ltable) {

    minavgmaxD_t stat[1];
    IgSelector *igs_ptr = new IgSelector(ltable, tgt);

    lgp_barrier();
    double tm = wall_seconds();
    hclib::finish([=]() {
      igs_ptr->start();
      for(int i=0;i<l_num_req;i++) {
          IgPkt pkt;
          pkt.idx = i;
          pkt.val = pckindx[i] >> 16;
          int dest_rank = pckindx[i] & 0xffff;
          igs_ptr->send(REQUEST, pkt, dest_rank);
      }
      igs_ptr->done(REQUEST); // Indicate that we are done with sending messages to the REQUEST mailbox
    });

    tm = wall_seconds() - tm;
    lgp_barrier();

    lgp_min_avg_max_d( stat, tm, THREADS );

    delete igs_ptr;
    return( stat->avg );
}

int64_t ig_check_and_zero(int64_t use_model, int64_t *tgt, int64_t *index, int64_t l_num_req) {
  int64_t errors=0;
  int64_t i;
  lgp_barrier();
  for(i=0; i<l_num_req; i++){
    if(tgt[i] != (-1)*(index[i] + 1)){
      errors++;
      if(errors < 5)  // print first five errors, report all the errors
        fprintf(stderr,"ERROR: model %ld: Thread %d: tgt[%ld] = %ld != %ld)\n",
                use_model,  MYTHREAD, i, tgt[i], (-1)*(index[i] + 1));
               //use_model,  MYTHREAD, i, tgt[i],(-1)*(i*THREADS+MYTHREAD + 1) );
    }
    tgt[i] = 0;
  }
  if( errors > 0 )
    fprintf(stderr,"ERROR: %ld: %ld total errors on thread %d\n", use_model, errors, MYTHREAD);
  lgp_barrier();
  return(errors);
}

int main(int argc, char * argv[]) {

  const char *deps[] = { "system", "bale_actor" };
  hclib::launch(deps, 2, [=] {

    //char hostname[1024];
    //hostname[1023] = '\0';
    //gethostname(hostname, 1023);
    //fprintf(stderr, "Hostname: %s rank: %d\n", hostname, MYTHREAD);

    int64_t i;
    int64_t ltab_siz = 100000;
    int64_t l_num_req  = 1000000;      // number of requests per thread
    int64_t num_errors = 0L, total_errors = 0L;
    int64_t printhelp = 0;

    int opt;
    while( (opt = getopt(argc, argv, "hn:T:")) != -1 ) {
      switch(opt) {
      case 'h': printhelp = 1; break;
      case 'n': sscanf(optarg,"%ld" ,&l_num_req);   break;
      case 'T': sscanf(optarg,"%ld" ,&ltab_siz);   break;
      default:  break;
      }
    }

    T0_fprintf(stderr,"Running ig on %d PEs\n", THREADS);
    T0_fprintf(stderr,"Number of Request / PE           (-n)= %ld\n", l_num_req );
    T0_fprintf(stderr,"Table size / PE                  (-T)= %ld\n", ltab_siz);

    // Allocate and populate the shared table array
    int64_t tab_siz = ltab_siz*THREADS;
    int64_t * table  = (int64_t*)lgp_all_alloc(tab_siz, sizeof(int64_t)); assert(table != NULL);
    int64_t *ltable  = lgp_local_part(int64_t, table);
    // fill the table with the negative of its shared index
    // so that checking is easy
    for(i=0; i<ltab_siz; i++)
      ltable[i] = (-1)*(i*THREADS + MYTHREAD + 1);

    // As in the histo example, index is used by the _agi version.
    // pckindx is used my the buffered versions
    int64_t *index   =  (int64_t*)calloc(l_num_req, sizeof(int64_t)); assert(index != NULL);
    int64_t *pckindx =  (int64_t*)calloc(l_num_req, sizeof(int64_t)); assert(pckindx != NULL);
    int64_t indx, lindx, pe;
    srand(MYTHREAD+ 5 );
    for(i = 0; i < l_num_req; i++){
      indx = rand() % tab_siz;
      index[i] = indx;
      lindx = find_local_index(indx);      // the distributed version of indx
      pe  = find_owner(indx);
      pckindx[i] = (lindx << 16) | (pe & 0xffff); // same thing stored as (local index, thread) "shmem style"
    }

    int64_t *tgt  =  (int64_t*)calloc(l_num_req, sizeof(int64_t)); assert(tgt != NULL);
  
    lgp_barrier();
  
    int64_t use_model;
    double laptime = 0.0;
    double injection_bw = 0.0;
  
          laptime = ig_selector(tgt, pckindx, l_num_req,  ltable);
  
       T0_fprintf(stderr,"  %8.3lf seconds\n", laptime);
  
#if USE_ERROR_CHECK
       num_errors += ig_check_and_zero(use_model, tgt, index, l_num_req);
  
    total_errors = num_errors;
    if( total_errors ) {
      T0_fprintf(stderr,"YOU FAILED!!!!\n");
    }
#endif // USE_ERROR_CHECK

    lgp_barrier();
    lgp_all_free(table);
    free(index);
    free(pckindx);
    free(tgt);

  });
  return 0;
}

