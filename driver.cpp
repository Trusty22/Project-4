#include "Shop.h"
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
using namespace std;

// Function prototype
void *barber(void *);
void *customer(void *);

// ThreadParam class
// This class is used as a way to pass more
// than one argument to a thread.
class ThreadParam {
public:
  ThreadParam(Shop *shop, int id, int service_time) : shop(shop), id(id), service_time(service_time) {};
  Shop *shop;
  int id;
  int service_time;
};

/*
 * Driver makes a barber shop with multiple barbNum and customers.
 * It uses threads for the producer-consumer realationship where arbers serve customers.
 *
 * After initialization of the threads and rading args we loop multiple times.
 * The first loop iterates through the number of barbNum, creating a thread for each barber.
 * Each thread contains the shop reference, barber ID, and service time.
 * The second loop handles the customers threads, usleep makes it randomly come in
 * (this will cause variation in mine vs test cases).
 * Each customer thread has a shop reference and customer ID.
 * After all customer threads are created another loop happens which waits for all customer threads to finish.
 * When the customers finish the next loop stops the infinite loops.
 * Finally, the last loop detaches all barbNum so we dont overflow all that info into a core dump.
 */

int main(int argc, char *argv[]) {

  // Read arguments from command line
  if (argc != 5) {
    cout << "Usage: num_barbNum num_chairs num_customers service_time" << endl;
    return -1;
  }

  int num_barbNum = atoi(argv[1]);
  int num_chairs = atoi(argv[2]);
  int num_customers = atoi(argv[3]);

  int service_time = atoi(argv[4]);

  // Multiple barbNum one shop many customers

  // Create arrays to hold thread identifiers for barbNum and customers.
  pthread_t bThread[num_barbNum];
  pthread_t cThread[num_customers];

  // Initializes the Shop object with n number of barbNum and chairs.
  Shop shop(num_barbNum, num_chairs);

  for (int i = 0; i < num_barbNum; i++) {
    ThreadParam *barber_param = new ThreadParam(&shop, i, service_time);
    pthread_create(&bThread[i], NULL, barber, barber_param);
  }

  for (int i = 0; i < num_customers; i++) {
    usleep(rand() % 1000);
    int id = i + 1;
    ThreadParam *customer_param = new ThreadParam(&shop, id, 0);
    pthread_create(&cThread[i], NULL, customer, customer_param);
  }

  //loops through all customers barbers to join, cancel, and detach
  for (int i = 0; i < num_customers; i++) {
    pthread_join(cThread[i], NULL);
  }

  for (int i = 0; i < num_barbNum; i++) {
    pthread_cancel(bThread[i]);
  }

  for (int i = 0; i < num_barbNum; i++) {
    pthread_detach(bThread[i]);
  }

  cout << "# customers who didn't receive a service = " << shop.get_cust_drops() << endl;
  return 0;
}
/*
    The Barber function implements all the barber threads.First we get need values from
    the ThreadParam structure barber_param, barber's ID, service time.
    Then we delete barber_param after we get the info to free up space.
    The barber enters an infinite loop, signalling readiness to serve a customer by calling
    shop.helloCustomer(bid), simulating the time spent serving the customer using
    usleep(service_time), and notifying the shop of its completion with shop.byeCustomer(bid).
    This loop makes a simulation of the work of a barber.
    The loop allows for outside interference like from customer.
*/
void *barber(void *arg) {
  ThreadParam *barber_param = (ThreadParam *)arg;
  Shop &shop = *barber_param->shop;

  int bid = barber_param->id;
  int service_time = barber_param->service_time;

  barber_param->shop = nullptr;
  delete barber_param;

  while (true) {
    shop.helloCustomer(bid);
    usleep(service_time);
    shop.byeCustomer(bid);
  }
  return nullptr;
}
/*
    Like the barber class customer gets the needed info like the customer ID
    shop and arguments. It also sets the current barber number to -1 for later use.
    Then it vists a shop and leaves when it gets set to -1 (no longer needed).
*/
void *customer(void *arg) {
  ThreadParam *customer_param = (ThreadParam *)arg;
  Shop &shop = *customer_param->shop;
  int cid = customer_param->id;
  int barbNum = -1;

  customer_param->shop = nullptr;
  delete customer_param;

  barbNum = shop.visitShop(cid);
  if (barbNum != -1) {
    shop.leaveShop(cid, barbNum);
  }
  return nullptr;
}