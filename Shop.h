#ifndef SHOP_ORG_H_
#define SHOP_ORG_H_

#include <iostream>
#include <pthread.h>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#define kDefaultNumChairs 3
#define kDeafultNumBarbers 1 // added for default barbers and for handeling defined cases.

class Shop {
public:
  // changed to include kDeafultNumBarbers and define some conditions
  Shop(int num_barbers, int num_chairs) : max_waiting_cust_((num_chairs > 0) ? num_chairs : kDefaultNumChairs),
                                          cust_drops_(0),
                                          num_barbers_((num_barbers > 0) ? num_barbers : kDeafultNumBarbers) {
    init();
  };
  // chnaged to define some conditions
  Shop() : max_waiting_cust_(kDefaultNumChairs),
           cust_drops_(0), num_barbers_(1) {
    init();
  };
  // return when a customer got a service, changed to int to match documention and make passing easier
  int visitShop(int customer_id);
  void leaveShop(int customer_id, int barber_id);
  void helloCustomer(int id);
  void byeCustomer(int id);
  int get_cust_drops() const;

private:
  const int max_waiting_cust_; // the max number of threads that can wait
  queue<int> waiting_chairs_;  // includes the ids of all waiting threads
  int cust_drops_;
  int num_barbers_; // Total number of barbers in the shop

  // Mutexes and condition variables to coordinate threads
  // mutex_ is used in conjuction with all conditional variables

  // I made a structure inspired by others on stack overflow and from the ThreadParam class.
  // this structer essentially makes it so that the condition variables can interact easily with the threads
  // It makes it so that each barber has their own threads based on the variables and
  // This way I dont need to make multiplue classes for a barber or a customer.
  // The struct will help handle that

  struct barber_cond {
    // allows for each barber to get their own condition variables
    barber_cond() {
      in_service_ = false;
      money_paid_ = false;
      customer_in_chair_ = 0;

      pthread_cond_init(&this->cond_barber_paid_, NULL);
      pthread_cond_init(&this->cond_customer_served_, NULL);
      pthread_cond_init(&this->cond_barber_sleeping_, NULL);
    }

    int customer_in_chair_; // Who is currently in the chair
    bool in_service_;
    bool money_paid_;

    pthread_cond_t cond_customer_served_;
    pthread_cond_t cond_barber_sleeping_;
    pthread_cond_t cond_barber_paid_;
  };

  pthread_cond_t cond_customers_waiting_;
  pthread_mutex_t mutex_;

  // This vector is how I keep track of each barbers conditions(wasy handeling for its variables)
  vector<barber_cond *> cond;

  int barber = 0;             // the id of the barber thread
  void setBarb(int id) const; // Use set to help with the changing of the current barber number, like a pointer for each threads barber.
  void getBarb() const;       // easy getting
  void init();
  void print(int person, string message);
  string int2string(int i);
};

#endif