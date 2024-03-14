#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <algorithm>
using namespace std;

vector<char> alphabets = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

struct Process {
    char id;
    int arrivalTime;
    int numCpuBursts;
    vector<int> cpuBurstTime;
    vector<int> ioBurstTime;
};

double next_exp(double lambda, int upperBound){
    double x;
    do
    {
        double r = drand48(); // Uniform dist [0.00,1.00)
        x = -log(r) / lambda; // Convert to exponential distribution
    } while (x > upperBound); // Skip values above the upperBound
    return x;
}

int randomIntInRange(int min, int max)
{
    return min + rand() % (max - min + 1);
}

int main(int argc, char *argv[]){

    if(argc<6){
        cerr<<"Not enough arguments"<<endl;
    }

    int num_processes = atoi(argv[1]);
    int num_cpu = atoi(argv[2]); //I/O or CPU bound
    long seed = atol(argv[3]);
    double lambda = atof(argv[4]);
    int upper_bound = atoi(argv[5]);

    srand48(seed);
    
    vector<Process> processes; 

    for (int i = 0; i < num_processes; ++i){
        Process p;
        char pid = alphabets[i];
        p.id = pid;
        
        int arrivalTime = floor(next_exp(lambda, upper_bound));

        p.arrivalTime = arrivalTime;
        while(1){
            p.numCpuBursts = ceil(drand48() * 64);
            if (p.numCpuBursts > upper_bound){
                continue;  
            }else{
                break;
            }
        }

        // cout<<p.numCpuBursts<<" "; 

        for(int j = 0; j < p.numCpuBursts; ++j){
            
            if(j != p.numCpuBursts - 1){
                int cpuBurstTime = ceil(next_exp(lambda, upper_bound));
                int ioBurstTime = ceil(next_exp(lambda, upper_bound)) * 10;
                if (i >= num_processes - num_cpu)
                {
                    cpuBurstTime *= 4;
                    ioBurstTime /= 8;
                }
                p.cpuBurstTime.push_back(cpuBurstTime);
                p.ioBurstTime.push_back(ioBurstTime);
            }else{
                int cpuBurstTime = ceil(next_exp(lambda, upper_bound));
                if (i >= num_processes - num_cpu)
                {
                    cpuBurstTime *= 4;
                    // ioBurstTime /= 8;
                }
                p.cpuBurstTime.push_back(cpuBurstTime);
            }
        }
        processes.push_back(p);
    }

    for (const Process &p : processes) {
        std::cout << "Process " << p.id << " - Arrival Time: " << p.arrivalTime << "ms; Num CPU Bursts: " << p.numCpuBursts << "\n";
        for (int i = 0; i < p.numCpuBursts; ++i)
        {
            std::cout << "--> CPU burst " << p.cpuBurstTime[i]<<"ms";
            if (i != p.numCpuBursts - 1)
            {
                std::cout << " --> I/O burst " << p.ioBurstTime[i]<<"ms";
            }
            std::cout << "\n";
        }
    }
}
