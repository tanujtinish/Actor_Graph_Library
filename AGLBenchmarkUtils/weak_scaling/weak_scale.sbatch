#!/bin/bash
#SBATCH -Jcustom_bfs_td
#SBATCH --account=gts-vsarkar9-forza
#SBATCH --mem=0
#SBATCH -N128 --ntasks-per-node=16
#SBATCH -t24:00:00
#SBATCH -qinferno
#SBATCH --output=TD_strong_scaling.out
#SBATCH --mail-user=alonkar@ucsc.edu

source ~/ActorGraphBenchmark/PACE/oshmem/oshmem-slurm.sh
export SKIP_VALIDATION=1
scale=26
for nodes in 1 2 4 8 16 32 64 128; 
do 
        echo "Number of Nodes: $nodes"
        cores=$((nodes*16))
        echo "Number of Cores: $cores"
        echo "Scale: $scale"
        srun -N$nodes -n$cores -c1 ./graph500_custom_bfs $scale
        echo ""
done