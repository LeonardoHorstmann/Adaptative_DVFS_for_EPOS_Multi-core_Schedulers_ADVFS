// EPOS Synchronizer Component Test Program

#include <utility/ostream.h>
#include <pmu.h>
#include <utility/handler.h>
#include <chronometer.h>
#include <thread.h>
#include <display.h>
#include <utility/random.h>
#include <semaphore.h>
#include <clock.h>
#include <machine/pc/rtc.h>
#include <alarm.h>

//#include <perf_mon.h>

using namespace EPOS;

OStream cout;
Spin _print_lock;

int teste(){

    int j = 0;

    int vetor[1000];
    int vetor2[1000];
    for (int i=0;i<1000;i++) {
         j += vetor2[i] + vetor[i];
         cout << "teste" << endl;
    }

    for (int i=0;i<1000;i++) {
         j += vetor2[i]*1 + vetor[i]+j;
         cout << "teste" << endl;
    }    
    return j;

}


int main()
{    
    Thread* cons[8];
    unsigned long long date_1 = RTC::date();
        for(int i=0;i<8;i++){
            Thread::Configuration   conf(Thread::READY,
                            Thread::Criterion(Thread::NORMAL,i)
                            );
            cons[i] = new Thread(conf, &teste);
   //         cons[i]->join();
        }

        for(int i=0; i<8;i++) {
            cons[i]->join();
        }

        for(int i = 0; i < 8; i++)
            delete cons[i];
/*
        Thread::_fim = true;

        cout << "\n\n\n\n\n TEMPERATURA:" << endl;
        for (int i = 0; i < Machine::n_cpus(); i++) {
            cout << "\n\n\n\n\ntempCPU" << i << endl; 
            for (int j = 0; j < Thread::_tempPos[i]; j++)
                    cout <<Thread::_graficoTemp[i][j] << endl;
        }

        cout << "\n\n\n\n\n INSTRUCTIONS RETIRED:" << endl;
        for (int i = 0; i < Machine::n_cpus(); i++) {
            cout << "\n\n\n\n\ninstCPU" << i << endl; 
            for (int j = 0; j < Thread::_tempPos[i]; j++)
                    cout <<Thread::_graficoInst[i][j] << endl;
        }

        cout << "\n\n\n\n\n LLC MISS:" << endl;
        for (int i = 0; i < Machine::n_cpus(); i++) {
            cout << "\n\n\n\n\nllcCPU" << i << endl; 
            for (int j = 0; j < Thread::_tempPos[i]; j++)
                    cout <<Thread::_graficoMiss[i][j] << endl;
        }
*/
    return 0;
}

