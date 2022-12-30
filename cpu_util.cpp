#include<stdio.h>
#include <unistd.h>
#include <string.h>
//the info of CPU util
typedef struct CPUPACKED 
{  
    char name[20];      
    unsigned int user; 
    unsigned int nice; 
    unsigned int system;
    unsigned int idle;  
    unsigned int iowait;  
    unsigned int irq;  
    unsigned int softirq;  
}CPU_OCCUPY;  

int get_cpuoccupy(CPU_OCCUPY *cpust1, CPU_OCCUPY *cpust2) 
{  
    FILE *fd;  
    char buff[256]; 
    char c;    

    fd = fopen("/proc/stat", "r");
    int i = 0;
    while(fscanf(fd,"%[^\n]%c", buff, &c)!=EOF){    // read line to buff
        if(i == 1) {    //fill the attributes of cpust1
            sscanf(buff, "%s %u %u %u %u %u %u %u", cpust1->name, &cpust1->user, &cpust1->nice, &cpust1->system,     
            &cpust1->idle, &cpust1->iowait, &cpust1->irq, &cpust1->softirq);
        } else if(i == 2){  //fill the attributes of cpust2
            sscanf(buff, "%s %u %u %u %u %u %u %u", cpust2->name, &cpust2->user, &cpust2->nice, &cpust2->system,     
            &cpust2->idle, &cpust2->iowait, &cpust2->irq, &cpust2->softirq);
            break;
        }  
        i++;
    } 
    fclose(fd);
    printf("the 2 cpu occupy status is 1: %s %u %u \n 2: %s %u %u \n", cpust1->name, cpust1->user, cpust1->nice, cpust2->name, cpust2->user, cpust2->nice);
    if(i == 2)  
        return 1;
    else 
        return 0;  
}

//Cal the cpuoccupy after sampling two timestamp of CPU data
double cal_cpuoccupy(CPU_OCCUPY *previous, CPU_OCCUPY *current)  
{  
    unsigned long od, nd;  
    double cpu_use = 0;  
    if(previous->user > current->user){
        printf("Wroing Sequence of inputs!!!\n");
        return -1;
    }
    if(strcmp(previous->name,current->name)){
        printf("Wroing CPU assignment!!!\n");
        return -1;
    }
    od = (unsigned long)(previous->user + previous->nice + previous->system + previous->idle + previous->iowait + previous->irq + previous->softirq);
    nd = (unsigned long)(current->user + current->nice + current->system + current->idle + current->iowait + current->irq + current->softirq);
    double sum = nd - od;  
    double leisure;  
    leisure = current->idle + current->iowait - previous->idle - previous->iowait;
    cpu_use = 100 - (100*leisure/sum);  
    //printf("%.2f\n",cpu_use);  
    return cpu_use;
}  

int ondemand_policy(bool pol_trig, CPU_OCCUPY *cpu1_stat1, CPU_OCCUPY *cpu2_stat1){       //the inputs arg refer to the previous cpu stat of util
      CPU_OCCUPY cpu1_stat2, cpu2_stat2; // current state of util

      if(!pol_trig){    //when pol_trig = false, it will run the initialization of CPU stat
            int res = get_cpuoccupy(cpu1_stat1, cpu2_stat1);
            if(res != 1)
                  return -1;
            printf("the init result of %s %s\n", cpu1_stat1->name, cpu2_stat1->name);
      } else {          // when pol_trig = true, it means that the policy runs normally, and it will update cpu stat1 everytime
            int res = get_cpuoccupy(&cpu1_stat2, &cpu2_stat2);          // get current state
            if(res != 1)
                  return -2;
            printf("the result of %s %s\n", cpu1_stat2.name, cpu2_stat2.name);
            double util1 = cal_cpuoccupy(cpu1_stat1, &cpu1_stat2);
            double util2 = cal_cpuoccupy(cpu2_stat1, &cpu2_stat2);
            if((util1 == -1) || (util2 == -1)){
                  return 0;
            }
            printf("the CPU0 util is %.2f, CPU1 util is %.2f \n", util1, util2);

            //------this section we execute the DVFS policy according to the util of different cores------
            //...
            //--------------------------------------------------------------------------------------------

            *cpu1_stat1 = cpu1_stat2;
            *cpu2_stat1 = cpu2_stat2;  //after executing the policy the current state becomes previous state and wait for next trig
      }

      return 1;
}

int main(){
    CPU_OCCUPY cpu1_stat1, cpu2_stat1;
    bool flag = false;
    for(int i = 0; i<16; i++){
        usleep(100*1000);
        int res = ondemand_policy(flag, &cpu1_stat1, &cpu2_stat1);
        if(res != 1){
            printf("It goes wrong!!! the res is %i, round %i \n", res, i);
            break;
        }
        flag = true;
    }

    return 0;
}
