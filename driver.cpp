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
 * Driver makes a barber shop with multiple barbers and customers.
 * It uses threads for the producer-consumer realationship where arbers serve customers.
 *
 * After initialization of the threads and rading args we loop multiple times.
 * The first loop iterates through the number of barbers, creating a thread for each barber.
 * Each thread contains the shop reference, barber ID, and service time.
 * The second loop handles the customers threads, usleep makes it randomly come in
 * (this will cause variation in mine vs test cases).
 * Each customer thread has a shop reference and customer ID.
 * After all customer threads are created another loop happens which waits for all customer threads to finish.
 * When the customers finish the next loop stops the infinite loops.
 * Finally, the last loop detaches all barbers so we dont overflow all that info into a core dump.
 */

int main(int argc, char *argv[]) {

  // Read arguments from command line
  if (argc != 5) {
    cout << "Usage: num_barbers num_chairs num_customers service_time" << endl;
    return -1;
  }

  int num_barbers = atoi(argv[1]);
  int num_chairs = atoi(argv[2]);
  int num_customers = atoi(argv[3]);

  int service_time = atoi(argv[4]);

  // Multiple barbers one shop many customers

  // Create arrays to hold thread identifiers for barbers and customers.
  pthread_t barber_threads[num_barbers];
  pthread_t customer_threads[num_customers];

  // Initializes the Shop object with n number of barbers and chairs.
  Shop shop(num_barbers, num_chairs);

  for (int i = 0; i < num_barbers; i++) {
    ThreadParam *barber_param = new ThreadParam(&shop, i, service_time);
    pthread_create(&barber_threads[i], NULL, barber, barber_param);
  }

  for (int i = 0; i < num_customers; i++) {
    usleep(rand() % 1000);
    int id = i + 1;
    ThreadParam *customer_param = new ThreadParam(&shop, id, 0);
    pthread_create(&customer_threads[i], NULL, customer, customer_param);
  }

  for (int i = 0; i < num_customers; i++) {
    pthread_join(customer_threads[i], NULL);
  }

  for (int i = 0; i < num_barbers; i++) {
    pthread_cancel(barber_threads[i]);
  }

  for (int i = 0; i < num_barbers; i++) {
    pthread_detach(barber_threads[i]);
  }

  cout << "# customers who didn't receive a service = " << shop.get_cust_drops() << endl;
  return 0;
}

void *barber(void *arg) {
  ThreadParam *barber_param = (ThreadParam *)arg;
  Shop &shop = *barber_param->shop;

  int barber_id = barber_param->id;
  int service_time = barber_param->service_time;

  barber_param->shop = nullptr;
  delete barber_param;

  while (true) {
    shop.helloCustomer(barber_id);
    usleep(service_time);
    shop.byeCustomer(barber_id);
  }
  return nullptr;
}

void *customer(void *arg) {
  ThreadParam *customer_param = (ThreadParam *)arg;
  Shop &shop = *customer_param->shop;
  int customerId = customer_param->id;

  customer_param->shop = nullptr;
  delete customer_param;

  int barbers = -1;
  barbers = shop.visitShop(customerId);
  if (barbers != -1) {
    shop.leaveShop(customerId, barbers);
  }
  return nullptr;
}