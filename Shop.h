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
#define kDeafultNumBarbers 1

class Shop {
public:
  Shop(int num_barbers, int num_chairs) : max_waiting_cust_((num_chairs > 0) ? num_chairs : kDefaultNumChairs),
                                          cust_drops_(0),
                                          num_barbers_((num_barbers > 0) ? num_barbers : kDeafultNumBarbers) {
    init();
  };

  Shop() : max_waiting_cust_(kDefaultNumChairs),
           cust_drops_(0), num_barbers_(1) {
    init();
  };

  int visitShop(int customer_id); // return true only when a customer got a service
  void leaveShop(int customer_id);
  void helloCustomer(int id);
  void byeCustomer(int id);
  int get_cust_drops() const;

private:
  const int max_waiting_cust_; // the max number of threads that can wait
  queue<int> waiting_chairs_;  // includes the ids of all waiting threads
  int cust_drops_;
  int num_barbers_; // Total number of barbers in the shop

  //
  // mutex_ is used in conjuction with all conditional variables

  struct barber_cond {
    // REDO
    //  Added so condition variables to coordinate threads are held in a struct conditions_struct
    //  allows for each barber to get their own condition variables
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
  vector<barber_cond *> cond; // vector used to give each barber their own set of condition variables

  static const int barber = 0; // the id of the barber thread
  int barb_number = -1;
  void init();
  string int2string(int i);
  void print(int person, string message);
  int getBarber();
};

#endif