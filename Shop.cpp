#include "Shop.h"
#include <string>

// creates mutex for the program. Creates a customer waiting variable.
void Shop::init() {
  pthread_mutex_init(&mutex_, NULL);
  pthread_cond_init(&cond_customers_waiting_, NULL);

  // Fills the vector list with condition structs so each barber will have their own vector.
  for (int i = 0; i < num_barbers_; i++) {
    barber_cond *new_struct = new barber_cond();
    cond.push_back(new_struct);
  }
}

string Shop::int2string(int i) {
  stringstream out;
  out << i;
  return out.str();
}
// Takes the absolute value of person and returns the correct positive value for customer and barber
void Shop::print(int person, string message) {
  cout << ((person > barber) ? "customer[" : "barber[") << abs(person) << "]: " << message << endl;
}

int Shop::get_cust_drops() const {
  return cust_drops_;
}

////////////////////////////////////////////////////////////////////////////////
int Shop::visitShop(int customer_id) {
  pthread_mutex_lock(&mutex_);
  // If all chairs are full, and no barber availble, then leave shop
  if (waiting_chairs_.size() == max_waiting_cust_) {
    print(customer_id, "leaves the shop because of no available waiting chairs.");
    ++cust_drops_;
    pthread_mutex_unlock(&mutex_);
    return -1;
  }

  // If someone is being served or transitioning waiting to service chair and no barbers are available
  // then take a chair and wait for service
  if (!waiting_chairs_.empty()) { //
    waiting_chairs_.push(customer_id);
    print(customer_id, "takes a waiting chair. # waiting seats available = " +
                           int2string(max_waiting_cust_ - waiting_chairs_.size()));
    pthread_cond_wait(&cond_customers_waiting_, &mutex_);
    waiting_chairs_.pop();
  }

  int openBarber = -1; // this will be returned if no barber is found, error value

  // finds an open barber and sets openbarber variable to that barbers index
  for (int i = 0; i < num_barbers_; i++) {
    if (cond[i]->customer_in_chair_ == 0) {
      openBarber = i;
    }
  }
  // Couldn't find barber, customer is put into the waiting queue
  if (openBarber == -1) {
    waiting_chairs_.push(customer_id);
    print(customer_id, "takes a waiting chair. # waiting seats available = " +
                           int2string(max_waiting_cust_ - waiting_chairs_.size()));
    pthread_cond_wait(&cond_customers_waiting_, &mutex_);
    waiting_chairs_.pop();
  }

  int choosen_barber = -1; // error value incase switching barbers has a problem

  // Assigns the customer to their barber for service. Sets chosen barber variable.
  for (int i = 0; i < num_barbers_; i++) {
    if (cond[i]->customer_in_chair_ == 0) {
      print(customer_id, "moves to service chair[" + to_string(i) + "]. # waiting seats available = " +
                             int2string(max_waiting_cust_ - waiting_chairs_.size()));
      cond[i]->customer_in_chair_ = customer_id;
      cond[i]->in_service_ = true;

      pthread_cond_signal(&cond[i]->cond_barber_sleeping_);
      choosen_barber = i;
      break;
    }
  }

  pthread_mutex_unlock(&mutex_);
  // cout << choosen_barber << endl;
  barb_number = choosen_barber;
  return choosen_barber; // returns the chosen barber index to service the customer.
}
// Method for when a customer is ready to leave the shop. Waits for hair cut to be done, pays barber and leaves. Customer
// thread terminates.
void Shop::leaveShop(int customer_id, int barber_id) {
  pthread_mutex_lock(&mutex_);
  string str = "wait for barber[" + int2string(barber_id) + "] to be done with the hair-cut";
  // Wait for service to be completed // needs to be wait for barber barber_id
  print(customer_id, str);
  while (cond[barber_id]->in_service_ == true) {
    pthread_cond_wait(&cond[barber_id]->cond_customer_served_, &mutex_);
  }

  // Pay the barber and signal barber appropriately
  cond[barber_id]->money_paid_ = true;
  pthread_cond_signal(&cond[barber_id]->cond_barber_paid_);
  string s = "says good-bye to the barber[" + int2string(barber_id) + "].";
  print(customer_id, s);
  pthread_mutex_unlock(&mutex_);
}
// Method dealing with the behavior of the barber. Takes in barber ID.
void Shop::helloCustomer(int id) {
  pthread_mutex_lock(&mutex_);

  // If no customers than barber can sleep
  if (waiting_chairs_.empty() && cond[id]->customer_in_chair_ == 0) {
    print(id, "sleeps because of no customers.");
    pthread_cond_wait(&cond[id]->cond_barber_sleeping_, &mutex_);
  }

  if (cond[id]->customer_in_chair_ == 0) // checks if a customer is currently sitting in the chair getting service
  {
    pthread_cond_wait(&cond[id]->cond_barber_sleeping_, &mutex_);
  }

  string str = "starts a hair-cut service for customer[" + int2string(cond[id]->customer_in_chair_) + "]";

  print(id, str);
  pthread_mutex_unlock(&mutex_);
}
// Method that takes in a barber id and cycles out the current customer after service is complete. Barber waits for
// payment. Once paid the customer leaves and now a new customer is signaled in.
void Shop::byeCustomer(int id) {
  pthread_mutex_lock(&mutex_);

  // Hair Cut-Service is done so signal customer that hair cut is done and wait for payment
  cond[id]->in_service_ = false;
  string str = "says he's done with a hair-cut service for customer[" + int2string(cond[id]->customer_in_chair_) + "]";
  print(id, str);
  cond[id]->money_paid_ = false;
  pthread_cond_signal(&cond[id]->cond_customer_served_);
  while (cond[id]->money_paid_ == false) {
    pthread_cond_wait(&cond[id]->cond_barber_paid_, &mutex_);
  }

  // Signal to next customer to take a seat after chair is empty.
  cond[id]->customer_in_chair_ = 0;
  print(id, "calls in another customer");
  pthread_cond_signal(&cond_customers_waiting_);

  pthread_mutex_unlock(&mutex_); // unlock
}