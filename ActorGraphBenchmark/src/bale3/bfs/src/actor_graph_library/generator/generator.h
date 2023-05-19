#ifndef BUILDER_H
#define BUILDER_H

#include "../../common.h"

#include "libgetput.h"
#include "selector.h"

#include "EdgeListDataStructure.h"
#include "./selectors/UniformGeneratorSelector.h"
#include "./selectors/RmatGeneratorSelector.h"
#include "./enums/GraphGeneratorTypeEnum.h"

#include <inttypes.h>
#include <assert.h>
#include <random>

class Generator {

	public:
		Generator(int edgefactor, int SCALE) : Generator(edgefactor, SCALE, UniformGenerator) {}
		
		Generator(int edgefactor, int SCALE, GraphGeneratorTypeEnum type){
			this->SCALE = SCALE; 
			this->edgefactor = edgefactor;
			this->type = type;
		}; 

		ResultFromGenerator GenerateEL(){
			ResultFromGenerator generatorResult;

			double make_graph_start = MPI_Wtime();

			if(type == UniformGenerator){
				if(my_pe() == 0){
                    printf("Generating Graph using 'Uniform Random' generator\n");
                }
				UniformGeneratorSelector *bfss_ptr = new UniformGeneratorSelector(edgefactor, SCALE, &generatorResult);
				hclib::finish([=] {
					bfss_ptr->start();
					bfss_ptr->send(0, {}, my_pe());
					bfss_ptr->done(0);
				});
				lgp_barrier();
			}
			else if (type == RmatGenerator){
				if(my_pe() == 0){
                    printf("Generating Graph using 'RMAT' generator\n");
                }
				RmatGeneratorSelector *bfss_ptr = new RmatGeneratorSelector(edgefactor, SCALE, &generatorResult);
				hclib::finish([=] {
					bfss_ptr->start();
					bfss_ptr->send(0, {}, my_pe());
					bfss_ptr->done(0);
				});
				lgp_barrier();
			}
			else{
				if(my_pe() == 0){
                    printf("Generating Graph using deafult 'Uniform Random' generator\n");
                }
				UniformGeneratorSelector *bfss_ptr = new UniformGeneratorSelector(edgefactor, SCALE, &generatorResult);
				hclib::finish([=] {
					bfss_ptr->start();
					bfss_ptr->send(0, {}, my_pe());
					bfss_ptr->done(0);
				});
				lgp_barrier();
			}

			double make_graph_stop = MPI_Wtime();
			generatorResult.make_graph_time = make_graph_stop - make_graph_start;

			return generatorResult;
		};

		tuple_graph getTG(ResultFromGenerator generatorResult){
			tuple_graph tg;
			tg.data_in_file = false;
			tg.edgememory = generatorResult.edgememory;
			tg.edgememory_size = generatorResult.edgememory_size;
			tg.max_edgememory_size = generatorResult.max_edgememory_size;
			tg.nglobaledges = (int64_t)(generatorResult.edgefactor) << generatorResult.SCALE;
			tg.weightmemory = generatorResult.weightmemory;
			
			return tg;
		};

		//getters/setters
		int getSCALE(){
			return this->SCALE;
		};
		int getEdgefactor(){
			return this->edgefactor;
		};

	private:
		int SCALE;
		int edgefactor;
		GraphGeneratorTypeEnum type;
};	
#endif