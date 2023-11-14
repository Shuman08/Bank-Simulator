#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_CUSTOMERS 1000

int m, tc, tw, td, ti;
int front = -1, rear = -1;

struct Customer
{
    int number;
    char service_type[2];
    time_t arrival_time;
};

struct Customer c[MAX_CUSTOMERS];
struct Customer c_queue[MAX_CUSTOMERS];

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; // to protect queue critical section
pthread_cond_t queue_not_empty_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_not_full_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t served_customers_mutex = PTHREAD_MUTEX_INITIALIZER; // to protect served_customer critical section

void *customer();
void *teller(void *arg);

int served_customers[4] = {0};  // Number of customers served by each teller
time_t start_time[4];           // Start time of each teller
time_t termination_time[4];     // Termination time of each teller
int total_customers_served = 0; // Total number of customers served by teller
int total_customers_read = 0;   // Total number of customers read from the file
int count = 0;

void *customer()
{
    FILE *fp;

    fp = fopen("c_file.txt", "r");
    if (fp == NULL)
    {
        perror("Failed to open customer or log file");
        exit(EXIT_FAILURE);
    }
    // read customer from file into queue
    while (fscanf(fp, "%d.%s", &c[count].number, c[count].service_type) != EOF)
    {
        // save customer arrival time
        time_t arrival_time = time(NULL);
        c[count].arrival_time = arrival_time;

         //acquires lock before entering shared data
        pthread_mutex_lock(&queue_mutex);

        //perform enqueue
        while (rear >= m - 1)
        {
            printf("\nQueue is Full!! Waiting for an empty slot...\n");
            // Waiting signal for queue is not full , then release the lock
            pthread_cond_wait(&queue_not_full_cond, &queue_mutex); 
        }

        rear++;
        c_queue[rear] = c[count];

        if (front == -1)
        {
            front = 0;
        }

        printf("\nCustomer Added to queue %d %s", c[count].number, c[count].service_type);
        total_customers_read++;

        // signal tellers that queue is no empty so they can serve the customer
        pthread_cond_signal(&queue_not_empty_cond);

        //release the lock on queue
        pthread_mutex_unlock(&queue_mutex);

        // reading customer every tc seconds
        sleep(tc);
        count++;
    }

    fclose(fp);

     //acquires lock on updating total customers read
    pthread_mutex_lock(&served_customers_mutex);
    total_customers_read = count;

    //release lock
    pthread_mutex_unlock(&served_customers_mutex);

    return NULL;
}

void *teller(void *arg)
{
    int teller_number = *(int *)arg;
    struct Customer curr_cus;

    while (1)
    {
        //acquires lock before entering shared data
        pthread_mutex_lock(&queue_mutex);

        while (front == -1)
        {
            printf("\nQueue is empty. Teller is waiting for a customer...");
            // wait for the signal  queue is not empty, then release the lock
            pthread_cond_wait(&queue_not_empty_cond, &queue_mutex);
        }

        // Get customer from the queue
        curr_cus = c_queue[front];

        // Perform dequeue
        if (front == rear)
        {
            front = rear = -1;
        }
        else
        {
            front++;
        }

        // Release the lock on the queue
        pthread_mutex_unlock(&queue_mutex);

        // Signal customer thread that the queue is not full so they can add customer to queue
        pthread_cond_signal(&queue_not_full_cond);

        //get customer arrival time 
        struct tm *timeinfo = localtime(&curr_cus.arrival_time);
        char a_string[100];
        strftime(a_string, sizeof(a_string), "%m/%d/%y %H:%M:%S", timeinfo);

        //write customer arrival time to the file
        FILE *log_file_i = fopen("r_log.txt", "a");
        if (log_file_i == NULL)
        {
            printf("\nError opening r_log file");
            exit(1);
        }
        fprintf(log_file_i, "Customer: %d\nService type: %s\nArrival time: %s\n-------------------------------------------------------------\n\n", curr_cus.number, curr_cus.service_type, a_string);
        fclose(log_file_i);

        //get customer response time
        time_t respond_time = time(NULL);
        timeinfo = localtime(&respond_time);
        char r_string[100];
        strftime(r_string, sizeof(r_string), "%m/%d/%y %H:%M:%S", timeinfo);

        //write customer response time to the file
        FILE *log_file_ii = fopen("r_log.txt", "a");
        if (log_file_ii == NULL)
        {
            printf("\nError opening r_log file");
            exit(1);
        }
        fprintf(log_file_ii, "Teller: %d\nCustomer: %d\nService type: %s\nArrival time: %s\nResponse time: %s\n=============================================================\n\n", teller_number, curr_cus.number, curr_cus.service_type, a_string, r_string);
        fclose(log_file_ii);

        // Teller serving customer based on the service type & duration
        switch (curr_cus.service_type[0])
        {
        // withdrawal 
        case 'W':

            printf("\nTeller %d is serving customer %d for withdrawal", teller_number, curr_cus.number);
            sleep(tw);
            printf("\nTeller %d finished serving customer %d for withdrawal. ", teller_number, curr_cus.number);
            break;
        // deposit
        case 'D':
            printf("\nTeller %d is serving customer %d for deposit", teller_number, curr_cus.number);
            sleep(td);
            printf("\nTeller %d finished serving customer %d for deposit", teller_number, curr_cus.number);
            break;
        // asking information
        case 'I':
            printf("\nTeller %d is serving customer %d for asking information", teller_number, curr_cus.number);
            sleep(ti);
            printf("\nTeller %d finished serving customer %d for asking information", teller_number, curr_cus.number);
            break;
        // invalid service , skipping the customer
        default:
            printf("\nTeller %d: Invalid service type %s for customer %d. Skipping...", teller_number, curr_cus.service_type, curr_cus.number);
            continue;
        }

        // get completion time of teller serving customer
        time_t completion_time = time(NULL);
        timeinfo = localtime(&completion_time);
        char c_string[100];
        strftime(c_string, sizeof(c_string), "%m/%d/%y %H:%M:%S", timeinfo);

        // Update the number of served customers for the teller
        served_customers[teller_number - 1]++;

        // Write completion time into log file
        FILE *log_file_iii = fopen("r_log.txt", "a");
        if (log_file_iii == NULL)
        {
            printf("\nError opening r_log file");
            exit(1);
        }

        fprintf(log_file_iii, "Teller: %d\nCustomer: %d\nService type: %s\nArrival time: %s\nCompletion time: %s\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", teller_number, curr_cus.number, curr_cus.service_type, a_string, c_string);
        fclose(log_file_iii);

        // acquire lock to check if all customer is served 
        pthread_mutex_lock(&served_customers_mutex);
        int all_customers_served = (total_customers_served == total_customers_read && front == -1);
        //release lock
        pthread_mutex_unlock(&served_customers_mutex);

        // if all customer served, threads can terminate
        if (all_customers_served)
        {
            // Update the termination time for the teller
            time(&termination_time[teller_number - 1]);

            // Cleanup and exit the thread
            pthread_mutex_unlock(&queue_mutex);
            pthread_exit(NULL);
        }
    }

    total_customers_served++;

    // Check if all customers have been served
    if (total_customers_served == count)
    {
        // Cleanup and exit the thread
        pthread_mutex_unlock(&queue_mutex);
        pthread_exit(NULL);
    }
}

int main(int argc, char *argv[])
{
    // if argument is not enough , show usage message 
    if (argc != 6)
    {
        printf("Usage: %s m tc tw td ti\n", argv[0]);
        printf("%s - your exe program\n");
        printf("m - the size of c_queue\n");
        printf("tc - seconds of adding customer to queue\n");
        printf("tw - seconds of withdrawal service\n");
        printf("td - seconds of deposit service\n");
        printf("ti - seconds of ask information service\n");
        return 1;
    }

    pthread_t customer_t, teller1, teller2, teller3, teller4;

    //set arguement value 
    m = atoi(argv[1]);
    tc = atoi(argv[2]);
    tw = atoi(argv[3]);
    td = atoi(argv[4]);
    ti = atoi(argv[5]);


    pthread_create(&customer_t, NULL, customer, NULL);

    pthread_create(&teller1, NULL, teller, (void *)&(int){1});
    pthread_create(&teller2, NULL, teller, (void *)&(int){2});
    pthread_create(&teller3, NULL, teller, (void *)&(int){3});
    pthread_create(&teller4, NULL, teller, (void *)&(int){4});

    // track start time for teller
    time(&start_time[0]); 
    time(&start_time[1]); 
    time(&start_time[2]); 
    time(&start_time[3]); 

    // wait for customer thread to terminate
    pthread_join(customer_t, NULL);
    printf("\n\nAll customer is added to the queue.");

    // acquire lock to access shared data 
    pthread_mutex_lock(&served_customers_mutex);
    total_customers_read = total_customers_served;
    pthread_mutex_unlock(&served_customers_mutex);

    // wait for teller to terminate
    pthread_join(teller1, NULL);
    printf("\n\nTeller 1 is terminated");
    pthread_join(teller2, NULL);
    printf("\n\nTeller 2 is terminated");
    pthread_join(teller3, NULL);
    printf("\n\nTeller 3 is terminated");
    pthread_join(teller4, NULL);
    printf("\n\nTeller 4 is terminated");

    printf("\n\nAll customer is served by teller 1,2,3,4.");

    //destory mutexes & condition variables used 
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&served_customers_mutex);
    pthread_cond_destroy(&queue_not_empty_cond);
    pthread_cond_destroy(&queue_not_full_cond);

    // Write teller information & statistics  to log file
    FILE *log_file_ii = fopen("r_log.txt", "a");
    if (log_file_ii == NULL)
    {
        printf("\nError opening r_log file");
        exit(1);
    }

    fprintf(log_file_ii, "\nTellers Information\n---------------------------------------\n");
    for (int i = 0; i < 4; i++)
    {
        struct tm *start_time_info = localtime(&start_time[i]);
        char start_time_str[100];
        strftime(start_time_str, sizeof(start_time_str), "%m/%d/%y %H:%M:%S", start_time_info);

        struct tm *termination_time_info = localtime(&termination_time[i]);
        char termination_time_str[100];
        strftime(termination_time_str, sizeof(termination_time_str), "%m/%d/%y %H:%M:%S", termination_time_info);

        fprintf(log_file_ii, "Teller-%d\n#served customers:%d\nStart time: %s\nTermination time: %s\n\n", i + 1, served_customers[i], start_time_str, termination_time_str);
    }

    fprintf(log_file_ii, "\nTeller Statistics\n---------------------------------------\n");
    for (int i = 0; i < 4; i++)
    {
        fprintf(log_file_ii, "Teller-%d serves %d customers.\n", i + 1, served_customers[i]);
    }
    fprintf(log_file_ii, "\nTotal number of customers: %d\n", count);
    fclose(log_file_ii);

    return 0;
}