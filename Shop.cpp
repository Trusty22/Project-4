
#include "Shop.h"

//creates mutex for the program. Creates a customer waiting variable.
void Shop::init() {
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_customers_waiting_, NULL);

    //Fills the vector list with condition structs so each barber will have their own vector.
    for(int i = 0; i < num_barbers_; i++){
        conditions_struct* new_struct = new conditions_struct();
        conditions.push_back(new_struct);
    }
}

string Shop::int2string(int i) {
    stringstream out;
    out << i;
    return out.str();
}
//Takes the absolute value of person and returns the correct positive value for customer and barber
void Shop::print(int person, string message) {
    cout << ((person > barber) ? "customer[" : "barber  [") << abs(person) << "]: " << message << endl;
}

int Shop::get_cust_drops() const {
    return cust_drops_;
}

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
    if (!waiting_chairs_.empty()) {
        waiting_chairs_.push(customer_id);
        print(customer_id, "takes a waiting chair. # waiting seats available = " +
                           int2string(max_waiting_cust_ - waiting_chairs_.size()));
        pthread_cond_wait(&cond_customers_waiting_, &mutex_);
        waiting_chairs_.pop();
    }

    int openBarber = -1; //this will be returned if no barber is found, error value

    //finds an open barber and sets openbarber variable to that barbers index
    for (int i = 0; i < num_barbers_; i++){
        if(conditions[i]->customer_in_chair_ == 0){
            openBarber = i;
        }
    }
    //Couldn't find barber, customer is put into the waiting queue
    if(openBarber == -1){
        waiting_chairs_.push(customer_id);
        print(customer_id, "takes a waiting chair. # waiting seats available = " +
                           int2string(max_waiting_cust_ - waiting_chairs_.size()));
        pthread_cond_wait(&cond_customers_waiting_, &mutex_);
        waiting_chairs_.pop();
    }

    int choosen_barber = -1; //error value incase switching barbers has a problem

    //Assigns the customer to their barber for service. Sets chosen barber variable.
    for(int i = 0; i < num_barbers_; i++){
        if(conditions[i]->customer_in_chair_ == 0){
            print(customer_id, "moves to the service chair [" + to_string(i) + "]. # waiting seats available = " +
                               int2string(max_waiting_cust_ - waiting_chairs_.size()));
            conditions[i]->customer_in_chair_ = customer_id;
            conditions[i]->in_service_ = true;

            pthread_cond_signal(&conditions[i]->cond_barber_sleeping_);
            choosen_barber = i;
            break;
        }
    }

    pthread_mutex_unlock(&mutex_);
    return choosen_barber; //returns the chosen barber index to service the customer.
}
//Method for when a customer is ready to leave the shop. Waits for hair cut to be done, pays barber and leaves. Customer
//thread terminates.
void Shop::leaveShop(int customer_id, int barber_id) {
    pthread_mutex_lock(&mutex_);

    // Wait for service to be completed
    print(customer_id, "wait for the hair-cut to be done");
    while (conditions[barber_id]->in_service_ == true) {
        pthread_cond_wait(&conditions[barber_id]->cond_customer_served_, &mutex_);
    }

    // Pay the barber and signal barber appropriately
    conditions[barber_id]->money_paid_ = true;
    pthread_cond_signal(&conditions[barber_id]->cond_barber_paid_);
    print(customer_id, "says good-bye to the barber.");
    pthread_mutex_unlock(&mutex_);
}
//Method dealing with the behavior of the barber. Takes in barber ID.
void Shop::helloCustomer(int id)
{
    pthread_mutex_lock(&mutex_);

    // If no customers than barber can sleep
    if (waiting_chairs_.empty() && conditions[id]->customer_in_chair_ == 0) {
        print(id, "sleeps because of no customers.");
        pthread_cond_wait(&conditions[id]->cond_barber_sleeping_, &mutex_);
    }

    if (conditions[id]->customer_in_chair_ == 0)  //checks if a customer is currently sitting in the chair getting service
    {
        pthread_cond_wait(&conditions[id]->cond_barber_sleeping_, &mutex_);
    }

    print(id, "starts a hair-cut service for " + int2string(conditions[id]->customer_in_chair_));
    pthread_mutex_unlock(&mutex_);
}
//Method that takes in a barber id and cycles out the current customer after service is complete. Barber waits for
//payment. Once paid the customer leaves and now a new customer is signaled in.
void Shop::byeCustomer(int id)
{
    pthread_mutex_lock(&mutex_);

    // Hair Cut-Service is done so signal customer that hair cut is done and wait for payment
    conditions[id]->in_service_ = false;
    print(id, "says he's done with a hair-cut service for " + int2string(conditions[id]->customer_in_chair_));
    conditions[id]->money_paid_ = false;
    pthread_cond_signal(&conditions[id]->cond_customer_served_);
    while (conditions[id]->money_paid_ == false) {
        pthread_cond_wait(&conditions[id]->cond_barber_paid_, &mutex_);
    }

    //Signal to next customer to take a seat after chair is empty.
    conditions[id]->customer_in_chair_ = 0;
    print(id, "calls in another customer");
    pthread_cond_signal(&cond_customers_waiting_);

    pthread_mutex_unlock(&mutex_);  // unlock
}
