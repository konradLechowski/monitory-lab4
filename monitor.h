#ifndef MONITOR_H
#define MONITOR_H

#include <semaphore.h>

class Semaphore 
{
    private:
        sem_t sem;

    public:
        Semaphore(int value) 
        {
            if( sem_init( &sem, 0, value ) != 0 )
                throw "sem_init: failed";
        }

        ~Semaphore()
        {
            sem_destroy( &sem );
        }

        void p( void )
        {
            if( sem_wait( & sem ) != 0 )
                throw "sem_wait: failed";
        }
        
        void v( void )
        {
            if( sem_post( & sem ) != 0 )
                throw "sem_post: failed";
        }
};

class Condition
{
    friend class Monitor;   //monitor can access methods of Condition
    
    private:
        Semaphore sem;
        int waitingCount;

    public:
        Condition() : sem(0)
        {
            waitingCount = 0;
        }

        void wait()
        {
            sem.p();
        }

        bool signal()
        {
            if( waitingCount )
            {
                --waitingCount;
                sem.v();
                return true;
            }
            else
                return false;
        }
};

class Monitor 
{
    private:
        Semaphore cs_1;    //critical section access to Q1
        Semaphore cs_2;    //critical section access to Q2

    public:
        Monitor() : cs_1( 1 ), cs_2( 1 ) {}   // init critical section as available

        void enter_first() { cs_1.p(); }

        void leave_first() { cs_1.v(); }

        void enter_second() { cs_2.p(); }

        void leave_second() { cs_2.v(); }

        void wait ( Condition &c, int queue )
        {
            c.waitingCount++;

            if(queue == 1)
                leave_first();
            else
                leave_second();
                
            c.wait();
        }

        void signal( Condition &c, int queue ) // Signal condition, if there's a thread waiting for this condition, enter critical section
        {
            if ( c.signal() )
            {
                if(queue == 1)
                    enter_first();
                else
                    enter_second();
            }
        }
};

#endif