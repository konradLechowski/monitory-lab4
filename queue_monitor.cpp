#include "queue_monitor.h"

// int p_id = 0;
// int c_id = 0;
// int r_id = 0;
 int m_id = 0;


RandomGen::RandomGen() : m_mt( (std::random_device()) () ) {}       //inits mersenne twister generator with random device

time_t RandomGen::getSleepTime(int max_wait_time)
{
    std::uniform_int_distribution<int> dist(0, max_wait_time);
    return dist(m_mt);                                              //cast int -> time_t (seconds)
}

int RandomGen::genQueueId()
{
    std::uniform_int_distribution<int> dist(1, 2);
    return dist(m_mt);                                              //returns either queue_id=1 or 2
}


void* Monitor_Q::printInvoker(void* context)
{
    return ( ( Monitor_Q*) context) -> printQ();    //bootstrap QPrint
}

void* Monitor_Q::producerInvoker(void* context)
{
    return ( (Monitor_Q*) context) -> produce();    //bootstrap producer
}

void* Monitor_Q::consumerInvoker(void* context)
{
    return ( (Monitor_Q*) context) -> consume();    //bootstrap consumer
}

void* Monitor_Q::readerInvoker(void* context)
{
    return ( (Monitor_Q*) context) -> read();       //bootstrap reader
}

void* Monitor_Q::printQ()
{
    while(true) {
    
        enter_first();                          //lock Q1

        pthread_mutex_lock(&ostream);           //lock ostream
        std::cout<<"Q1:";
        for(int i = 0; i < Q1.size(); i++)
        {
            std::cout<<" " << Q1[i].msg_id;     //print Q1 elements
        }
        std::fflush;
        pthread_mutex_unlock(&ostream);         //unlock ostream for (possibly) other processes

        leave_first();                          //unlock Q1

        enter_second();                         //lock Q2

        pthread_mutex_lock(&ostream);           

        std::cout<<"\nQ2:";
        for(int j = 0; j < Q2.size(); j++)
        {
            std::cout<<" "<<Q2[j].msg_id;
        }
        std::cout<<std::endl<<std::fflush;

        pthread_mutex_unlock(&ostream);

        leave_second();
        

        //sleep for 10s idk
        std::this_thread::sleep_for(std::chrono::seconds(10)); 
    }
}

void* Monitor_Q::produce()
{
    message msg;
    msg.prod_id = ++p_id;
    time_t sleep_time;

    //sleep on awakening for a few seconds
    std::this_thread::sleep_for(std::chrono::seconds(rg.getSleepTime(10)));

    while (true) {

        msg.msg_id = ++m_id;                            //next message id
        msg.queue_id = rg.genQueueId();                 //generate random queue for the message
        if ( msg.queue_id == 1)
        {

            enter_first();                              //lock Q1
            if ( Q1.size() >= S )                       //if reached max size, block any further operations and wait for signal of empty space
                wait(Q1_has_empty, msg.queue_id);       //unlocks Q1 for another thread

            Q1.push_back(msg);                          //produce msg for Q1


            pthread_mutex_lock(&ostream);               //lock ostream, print message
            std::cout <<"P-"<< msg.prod_id << " added msg-"<< msg.msg_id<<" to Q"<<msg.queue_id<<std::endl <<std::fflush;
            std::this_thread::sleep_for(std::chrono::milliseconds(40)); //wait for roughly 2 frames, to avoid artifacts in console
            pthread_mutex_unlock(&ostream);             //unlock ostream

            leave_first();                              //unlock Q1
            signal ( Q1_has_elements, msg.queue_id );   //signal for waiting consumers, that Q1 has elements, if any thread is waiting
                                                        //signal() locks Queue
        } 
        else
        {
            enter_second();
            
            if ( Q2.size() >= S )
                wait(Q2_has_empty, msg.queue_id);

            Q2.push_back(msg);

            pthread_mutex_lock(&ostream);
            std::cout <<"P-"<< msg.prod_id << " added msg-"<< msg.msg_id<<" to Q"<<msg.queue_id<<std::endl << std::fflush;
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            pthread_mutex_unlock(&ostream);


            leave_second();
            signal( Q2_has_elements, msg.queue_id );
            
        }


        sleep_time = rg.getSleepTime(P_WAIT_TIME);                      //generate random sleep time
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));  //sleep
    }
}

void* Monitor_Q::consume()
{
    int cons_id = ++c_id;

    message m1;
    message m2;
    m1.queue_id = 1;
    m2.queue_id = 2;

    time_t sleep_time;

    //sleep for a few seconds on awakening (to not awake all consumers at once)
    std::this_thread::sleep_for(std::chrono::seconds(rg.getSleepTime(10)));

    while (true) 
    {

        enter_first();                                  //lock Q1 from other threads
        if (Q1.size() == 0)                             
            wait(Q1_has_elements, m1.queue_id );        //stop and wait for signal if no elements are in the Q
            
        
        m1.msg_id = Q1.front().msg_id;                  //copy values, pop message from queue
        m1.prod_id = Q1.front().prod_id;
        Q1.pop_front();
        
        pthread_mutex_lock(&ostream);                   //lock ostream, print msg
        std::cout << "C-" << cons_id << " consumed msg-" << m1.msg_id << " from P-" << m1.prod_id << std::endl << std::fflush;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        pthread_mutex_unlock(&ostream);                 //sleep for 2+ frames, unlock ostream

        leave_first();                                  //unlock Q1
        signal ( Q1_has_empty, m1.queue_id );           //signal that there are empty slots available in Q
                                                        //if producers are waiting for slots, signal() also locks Q1
        enter_second();                                 //identical as above
        if(Q2.size() == 0)
            wait(Q2_has_elements, m2.queue_id );
        
        m2.msg_id = Q2.front().msg_id;
        m2.prod_id = Q2.front().prod_id;
        Q2.pop_front();

        pthread_mutex_lock(&ostream);
        std::cout << "C-" << cons_id << " consumed msg-" << m2.msg_id << " from P-" << m2.prod_id << std::endl << std::fflush;
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        pthread_mutex_unlock(&ostream);

        leave_second();
        signal ( Q2_has_empty, m2.queue_id );

        
        
        sleep_time = rg.getSleepTime(C_WAIT_TIME);
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
    }

}

void* Monitor_Q::read()
{

    int reader_id = ++r_id;
    int queue_id = rg.genQueueId();                         //reader is defined throughout runtime with Q from which he will read msgs

    message msg;
    msg.queue_id = queue_id;

    time_t sleep_time;                                      //sleep at awakening not to invoke all readers at once
    std::this_thread::sleep_for(std::chrono::seconds(rg.getSleepTime(10)));


    while (true)
    {
        if (queue_id == 1)
        {
            enter_first();                                  //lock Q1 from modifying

            if(Q1.size() == 0)                              //unlock Q1 if no elements are there, and wait
                wait( Q1_has_elements, queue_id );

            msg.msg_id = Q1.back().msg_id;                  //copy values ('read from q')
            msg.prod_id = Q1.back().prod_id;

            pthread_mutex_lock(&ostream);                   //lock ostream, print read msg
            std::cout<<"R-" << reader_id << " read msg-"<<msg.msg_id<<" from Q"<<queue_id<<std::endl<<std::fflush;
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            pthread_mutex_unlock(&ostream);                 //sleep for 2+ frames and unlock ostream
        
            leave_first();                                  //unlock Q1
            signal( Q1_has_elements, queue_id );            //signal for consumers waiting that the queue still has elements
                                                            //if there's a consumer waiting, lock Q1 for the next thread
            

        } else {                                            //exact same instructions as for Q1

            enter_second();

            if(Q2.size() == 0)
                wait( Q2_has_elements, queue_id );

            msg.msg_id = Q2.back().msg_id;
            msg.prod_id = Q2.back().prod_id;

            pthread_mutex_lock(&ostream);
            std::cout<<"R-" << reader_id << " read msg-"<<msg.msg_id<<" from Q"<<queue_id<<std::endl<<std::fflush;
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            pthread_mutex_unlock(&ostream);

            leave_second();
            signal( Q2_has_elements, queue_id );
        }

                                                            //generate sleep time and goto sleep
        sleep_time = rg.getSleepTime(R_WAIT_TIME);
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
    }
    
}