//Adarsh Kurian
//CS537
//Bank simulation with pthreads
//5/4/17
//COMPILE USING: gcc -pthread
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>

static int N,D,L,T,transactions,days;
static bool * tellerLock;
static double * accountArray;
static double * pocketArray;
static double * tellerArray;
sem_t teller;
pthread_mutex_t update;
pthread_mutex_t change;
pthread_mutex_t get;

struct customer_info{
    int id;
};

int getTeller(){
    int i;
    for(i =0; i < T; ++i){
        if(tellerLock[i] == false){
            tellerLock[i] = true;
            return i;
        }
    }
    return -1;
}

int getTransactionType(){
    int temp = rand()%4;
    if (temp == 3){ 
        temp = 2;
    }
    return temp;// 0 = transfer, 1 = withdrawal, 2 = deposit
}

double getTransactionAmount(){
    return (rand()%100000)/100.0;
}

void doTransaction(int transCode, double transAmount, double * accountArray, double * pocketArray, double * tellerArray, int CID, int TID){
   int target = -1;
   double account, pocket, netAmount, teller, targetAccount;
   target = rand() % N;

    //you may insert synchronization code here
    pthread_mutex_lock(&update);

    pocket= pocketArray[CID];
    account= accountArray[CID];
    teller= tellerArray[TID];
    targetAccount= accountArray[target];

    if(transCode > 0){
        netAmount = (2 * transCode - 3) * transAmount; 
        if((accountArray[CID] + netAmount) > 0){
            pocket = pocketArray[CID] - netAmount;
            account = accountArray[CID] + netAmount;
            teller = tellerArray[TID] + netAmount;
        }
    }
    else if((accountArray[CID] - transAmount) > 0){
        account = accountArray[CID] - transAmount;
        targetAccount = accountArray[target] + transAmount;

    }

    if(transCode > 0){
        pocketArray[CID] = pocket;
        accountArray[CID] = account;
        tellerArray[TID] = teller;
    }

    else{
        accountArray[CID] = account;
        accountArray[target] = targetAccount;
    }
    ++transactions; //increment transactions
    //you may insert synchronization code here
    pthread_mutex_unlock(&update);

}

void * customer (void * info){
    struct customer_info * c = (struct customer_info *)info;
    int CID = c->id;
    //int x =0;                            //THREAD TESTING CODE(uncomment 96 - 98 to see thread starting)
    //sem_getvalue(&teller, &x);
    //printf("\nThread[%d] started. Sem value : %d. Day: %d. Trans num: %d\n", CID, x,days,transactions);
    while (true) {
        sem_wait(&teller);  //sem wait
        int TID = -1;
        pthread_mutex_lock(&get);
        while((TID = getTeller()) == -1){} //lock teller and retrieve teller ID
        pthread_mutex_unlock(&get);
        int transCount = rand()%L + 1;
        int i;
        for (i = 0; i < transCount; i++){
            while(transactions >= N){} // spinlock/sleep for the day
            int transCode = getTransactionType();
            int transAmount = getTransactionAmount();
            doTransaction(transCode, transAmount, accountArray, pocketArray, tellerArray, CID, TID);
        }
        pthread_mutex_lock(&get);
        tellerLock[TID] = false; //unlock teller
        pthread_mutex_unlock(&get);
        sem_post(&teller); //sem signal
    }
}

int main(int argc, char ** argv){
    if(argc != 5){
        printf("\nNot enough arguments. Expected: N customers, D days, L transactions, T tellers\n");
        return 0;
    }
    
    N = atoi(argv[1]);
    if(N <= 0){
        printf("\nN must be positive integer\n");
        return 0;
    }
    D = atoi(argv[2]);
    if(D <= 0){
        printf("\nD must be positive integer\n");
        return 0;
    }
    L = atoi(argv[3]);
    if(L <= 0){
        printf("\nL must be positive integer\n");
        return 0;
    }
    T = atoi(argv[4]);
    if(T <= 0){
        printf("\nT must be positive integer\n");
        return 0;
    }
    
    printf("\nN: %d D: %d L: %d T: %d\n\n", N,D,L,T);

    int i;

    tellerLock = (bool *)malloc(sizeof(bool) * T);          //initialize all arrays
    accountArray = (double *)malloc(sizeof(double) * N);
    pocketArray = (double *)malloc(sizeof(double) * N);
    tellerArray = (double *)malloc(sizeof(double) * T);

    for(i =0; i< T; ++i){
        tellerLock[i] = false;
    }

    transactions = 0;
    days =0;
    pthread_t threads[N];   //initialize thread array
    time_t t;
    srand((unsigned) time(&t)); //seed random gen

    if(pthread_mutex_init(&update, NULL) != 0){             //initialize account update mutex
        printf("\nCould not initialize mutex: Update\n");
        return -1;
    }

    if(pthread_mutex_init(&get, NULL) != 0){                //initialize teller get mutex
        printf("\nCould not initialize mutex: Get\n");
        return -1;
    }

    if(sem_init(&teller, 0, T)){                            //initialize counting semaphore to number of tellers
        printf("\nCould not initialize a semaphore\n");
        return -1;
    }

    for(i = 0; i < N; ++i){
       struct customer_info * x = (struct customer_info *)malloc(sizeof(struct customer_info));
       x->id = i; 
       if(pthread_create(&threads[i], NULL, &customer, x)){        //initialize all threads
            printf("\nThread %d creation error\n", i);
            return -1;
       }
    }
    double totalInterest = 0.0;
    while(days < D){
        if(transactions >= N){
            for(i =0; i< N; ++i){
                totalInterest += ((3 * accountArray[i]/10000));
                accountArray[i] += ((3 * accountArray[i]/10000)); //add interest to all accounts
            }
            ++days;
            transactions =0;
        }
    }
    for(i = 0; i < N; ++i){
       if(pthread_cancel(threads[i])){
            printf("\nThread %d creation error\n", i);      //cancel all threads
            return -1;
       }
    }
    printf("\nAccounts: ");                             //print all info
    for(i =0; i < N; ++i){
        printf("\nAccount[%d]: %f ", i, accountArray[i]);
    }
    printf("\nPockets: ");
    for(i =0; i < N; ++i){
        printf("\nPocket[%d]: %f ", i, pocketArray[i]);
    }
    printf("\nTellers: ");
    for(i =0; i < T; ++i){
        printf("\nTeller[%d]: %f ", i, tellerArray[i]);
    }
    printf("\nTotal interest paid: %f ", totalInterest);

    double totals = 0.0;
    for(i =0; i < N; ++i){
        totals += accountArray[i];
    }
    printf("\nTotal account balances: %f ", totals);
    totals = 0.0;
    for(i =0; i < N; ++i){
        totals += pocketArray[i];
    }
    printf("\nTotal net change pocket cash: %f ", totals);
    totals = 0.0;
    for(i =0; i < N; ++i){
        totals += tellerArray[i];
    }
    printf("\nTotal net teller transactions: %f ", totals);
    printf("\n\n");

    sem_destroy(&teller);
    pthread_mutex_destroy(&update);
    pthread_mutex_destroy(&get);
    free(tellerArray);
    free(tellerLock);
    free(pocketArray);
    free(accountArray);
    return 0;
}