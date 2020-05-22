#include "queue_monitor.h"
//#include <thread>



int main()
{

    pthread_t prod[P], cons[C], read[R], bufread;
    int i;
    Monitor_Q m;

    for ( i = 0; i < P; i++ )
    {
        pthread_create(&prod[i], NULL, &Monitor_Q::producerInvoker, &m );
    }
    //std::this_thread::sleep_for(std::chrono::seconds(2));
    for ( i = 0; i < C; i++ )
    {
        pthread_create(&cons[i], NULL, &Monitor_Q::consumerInvoker, &m );
    }
    //std::this_thread::sleep_for(std::chrono::seconds(2));

    for ( i = 0; i < R; i++ )
    {
        pthread_create(&read[i], NULL, &Monitor_Q::readerInvoker, &m );
    }

    pthread_create(&bufread, NULL, &Monitor_Q::printInvoker, &m);


    for ( i = 0; i < P; i++ )
    {
        pthread_join(prod[i], NULL);
    }

    for ( i = 0; i < C; i++ )
    {
        pthread_join(cons[i], NULL);
    }

    for ( i = 0; i < R; i++ )
    {
        pthread_join(read[i], NULL);
    }
    pthread_join(bufread, NULL);
    return 0;
}