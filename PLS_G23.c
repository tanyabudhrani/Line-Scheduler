#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// constant values
#define ORDER_ID 10
#define PRODUCT_NAME 10
#define DATE 11

// a structure for the orders in the system
typedef struct Order {
    char order_number[ORDER_ID], due_date[DATE], product_name[PRODUCT_NAME];
    int quantity;
    struct Order* next;
} Order;

// a structure for the scheduling period
typedef struct Period {
    char start_date[DATE], end_date[DATE];
} Period;

// a structure for the information about each plant
typedef struct Plant{
    char name[50];
    int capacity;
    int days;
    int produce; 
} Plant;

// a structure for the shared data for the SJF function
typedef struct {
    int days[3];   
    int produce[3];    
    int orderQuantities[];  
} SharedData;


// global variables
static Order* head = NULL;
static Period scheduling_period;
int numOrders = 0;
int numberOfDays = 0;
Plant plantsArray[3] = {{"Plant_X", 300, 0, 0}, {"Plant_Y", 400, 0, 0},{"Plant_Z", 500, 0, 0}} ;
Order* rejectedOrders[100];
int rejectedCount = 0;


// function for checking if order is added and batch file is added
void printOrderHistory() {

    Order* current = head;

    if (current == NULL) {

        printf("No Order in the history.\n");
        return;
    }
    
    printf("Order History:\n");
    printf("%-10s | %-10s | %-8s | %-20s\n", "Order ID", "Due Date", "Quantity", "Product Name");
    printf("-------------------------------------------------\n");
    
    while (current != NULL) {
        
        printf("%-10s | %-10s | %-8d | %-20s\n",
               current->order_number,
               current->due_date,
               current->quantity,
               current->product_name);
        current = current->next;
    }
}

// function to add an order to the rejected list if it cannot be executed
void rejectOrder(Order* order) {
    rejectedOrders[rejectedCount++] = order;
    printf("Order %s rejected as it cannot be completed by its due date %s.\n", order->order_number, order->due_date);
}

// function to add scheduling period
void period(const char* start_date, const char* end_date) {

    strcpy(scheduling_period.start_date, start_date);
    strcpy(scheduling_period.end_date, end_date);
    printf("Scheduling period set succesfully (%s to %s)\n", start_date, end_date);

}

// function to calculate the number of days in the scheduling period 
void calculateDays(const char* start_date, const char* end_date) {
    struct tm tm_start, tm_end;
    time_t time_start, time_end;

    memset(&tm_start, 0, sizeof(struct tm));
    memset(&tm_end, 0, sizeof(struct tm));

    if (strptime(start_date, "%Y-%m-%d", &tm_start) == NULL) {
        printf("Error parsing start date.\n");
        numberOfDays = -1;
    }
    if (strptime(end_date, "%Y-%m-%d", &tm_end) == NULL) {
        printf("Error parsing end date.\n");
        numberOfDays = -1;
    }
    time_start = mktime(&tm_start);
    time_end = mktime(&tm_end);

    if (time_start == -1 || time_end == -1) {
        printf("Error converting dates to time.\n");
        numberOfDays = -1;
    }

    double seconds_diff = difftime(time_end, time_start);
    int days_diff = (int)(seconds_diff / (24 * 3600));

    numberOfDays = days_diff;
}

// function to calculate the difference in days between any two dates
int calculateNumberOfDays(const char* start_date, const char* end_date) {
    struct tm tm_start, tm_end;
    time_t time_start, time_end;

    memset(&tm_start, 0, sizeof(struct tm));
    memset(&tm_end, 0, sizeof(struct tm));

    if (strptime(start_date, "%Y-%m-%d", &tm_start) == NULL) {
        printf("Error parsing start date.\n");
        return -1;
    }
    if (strptime(end_date, "%Y-%m-%d", &tm_end) == NULL) {
        printf("Error parsing end date.\n");
        return -1;
    }

    time_start = mktime(&tm_start);
    time_end = mktime(&tm_end);

    if (time_start == -1 || time_end == -1) {
        printf("Error converting dates to time.\n");
        return -1;
    }

    double seconds_diff = difftime(time_end, time_start);
    int days_diff = (int)(seconds_diff / (24 * 3600));

    return days_diff;
}

// helper function to check if an order can be completed within its due date (if needs to be rejected of not)
bool canCompleteOrder(Order* order, int plantCapacity, const char* currentDate) {
    int daysToDue = calculateNumberOfDays(currentDate, order->due_date);
    int totalProductionNeeded = (order->quantity + plantCapacity - 1) / plantCapacity; 
    return totalProductionNeeded <= daysToDue;
}

// function to add an order or multiple lines of order
void order(const char* order_number, const char* due_date, int quantity, const char* product_name) {
    
    Order* current = head;
    while (current != NULL) {
        
        if (strcmp(current->order_number, order_number) == 0) {
            printf("Order number: %s already exists. Duplicate not added.\n", order_number);
            return; 
            
        }

        current = current->next;
    }

    Order* new_order = (Order*)malloc(sizeof(Order));
    strcpy(new_order->order_number, order_number);
    strcpy(new_order->due_date, due_date);
    new_order->quantity = quantity;
    strcpy(new_order->product_name, product_name);
    new_order->next = NULL;

    if (head == NULL) {

        head = new_order;

    } else {

        current = head;

        while (current->next != NULL) {
            current = current->next;
        }

        current->next = new_order;
    }

    printf("Order Successful (%s).\n", order_number);
}

// helper function to sort the orders using bubble sort for shortest job first scheduling algorithm
void bubbleSortOrders(Order **head) {
    int swapped;
    Order *ptr1;
    Order *lptr = NULL;
    Order *temp;

    if (*head == NULL) return;

    do {
        swapped = 0;
        ptr1 = *head;

        while (ptr1->next != lptr) {
            if (ptr1->quantity > ptr1->next->quantity) {
                // swap
                Order *tmp = ptr1->next;
                ptr1->next = tmp->next;
                tmp->next = ptr1;

                if (ptr1 == *head) {
                    *head = tmp;
                } else {
                    Order *prev = *head;
                    while (prev->next != ptr1) prev = prev->next;
                    prev->next = tmp;
                }

                ptr1 = tmp;
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
}

// helper function to count the number of remaining elements
int countOrders(Order *head) {
    int count = 0;
    while (head) {
        count++;
        head = head->next;
    }
    return count;
}

// function to print the report into a file 
void printReport(const char* algorithm, const char* fileName) {
    FILE* report_file = fopen(fileName, "w");
    if (report_file == NULL) {
        printf("Error opening report file.\n");
        return;
    }

    fprintf(report_file, "***PLS Schedule Analysis Report***\n");
    fprintf(report_file, "Algorithm used: %s\n", algorithm);

    fprintf(report_file, "There are %d Orders ACCEPTED. Details are as follows:\n", (numOrders - rejectedCount));
    fprintf(report_file, "ORDER NUMBER START END DAYS QUANTITY PLANT\n");
    fprintf(report_file, "===========================================================================\n");

    Order* current = head;
    while (current != NULL) {
        fprintf(report_file, "%s %s %s %d\n", current->order_number, scheduling_period.start_date, scheduling_period.end_date, current->quantity);
        current = current->next;
    }
    fprintf(report_file, "- End -\n");
    fprintf(report_file, "===========================================================================\n\n");

    fprintf(report_file, "There are %d Orders REJECTED. Details are as follows:\n", rejectedCount);
    fprintf(report_file, "ORDER NUMBER PRODUCT NAME Due Date QUANTITY\n");
    fprintf(report_file, "===========================================================================\n");

    for (int i = 0; i < rejectedCount; i++) {
        fprintf(report_file, "%s %s %s %d\n", rejectedOrders[i]->order_number, rejectedOrders[i]->product_name, rejectedOrders[i]->due_date, rejectedOrders[i]->quantity);
    }
    fprintf(report_file, "- End -\n");
    fprintf(report_file, "===========================================================================\n\n");

    fprintf(report_file, "***PERFORMANCE\n\n");

    double totalProduce = 0, totalCapacity = 0;
    for (int i = 0; i < 3; i++) {
        int totalProduceForPlant = plantsArray[i].capacity * plantsArray[i].days;
        totalCapacity += totalProduceForPlant;
        double plantUtil = totalProduceForPlant == 0 ? 0 : (double)plantsArray[i].produce / totalProduceForPlant;
        fprintf(report_file, "%s:\n", plantsArray[i].name);
        fprintf(report_file, "Number of days in use: %d days\n", plantsArray[i].days);
        fprintf(report_file, "Number of products produced: %d (in total)\n", plantsArray[i].produce);
        fprintf(report_file, "Utilization of the plant: %.2f %%\n\n", plantUtil * 100);
        totalProduce += plantsArray[i].produce;
    }

    double overallUtil = totalCapacity == 0 ? 0 : (totalProduce / totalCapacity);
    fprintf(report_file, "Overall utilization: %.2f %%\n\n", overallUtil * 100);

    fclose(report_file);
    printf("Report generated successfully.\n");
}

// helper function to assign orders to plants using SJF
void assignOrdersToPlantsSJF(Order* head, const char* fileName, const char* algorithm) {
    if (!head) return;

    int orderCount = countOrders(head);
    Order* orderArray[orderCount];
    int index = 0;
    for (Order* current = head; current != NULL; current = current->next) {
        if (canCompleteOrder(current, plantsArray[0].capacity, scheduling_period.start_date)) {
            orderArray[index++] = current;
        } else {
            rejectOrder(current);
        }
    }

    int shmid = shmget(IPC_PRIVATE, sizeof(SharedData) + sizeof(int) * orderCount, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    SharedData* sharedData = (SharedData*)shmat(shmid, NULL, 0);
    if (sharedData == (SharedData*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }


    memset(sharedData->days, 0, sizeof(sharedData->days));
    memset(sharedData->produce, 0, sizeof(sharedData->produce));
    for (int i = 0; i < orderCount; i++) {
        sharedData->orderQuantities[i] = orderArray[i]->quantity;
    }

    int numDays = numberOfDays;
    pid_t pids[numDays];
    for (int day = 0; day < numDays; day++) {
        pids[day] = fork();
        if (pids[day] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pids[day] == 0) { 
            printf("Day %d:\n", day + 1);

            int availableOrders[orderCount];
            memset(availableOrders, 0, sizeof(availableOrders));
            int availablePlants[3] = {0, 0, 0};

            while (1) {
                double minTime = 1e9;
                int minPlant = -1, minOrder = -1;

                for (int i = 0; i < 3; i++) {
                    if (!availablePlants[i]) {
                        for (int j = 0; j < orderCount; j++) {
                            if (!availableOrders[j] && sharedData->orderQuantities[j] > 0) {
                                double processingTime = (double)sharedData->orderQuantities[j] / plantsArray[i].capacity;
                                if (processingTime < minTime) {
                                    minTime = processingTime;
                                    minPlant = i;
                                    minOrder = j;
                                }
                            }
                        }
                    }
                }

                if (minPlant == -1 || minOrder == -1) break;

                int dailyProduction = plantsArray[minPlant].capacity;
                int production = (sharedData->orderQuantities[minOrder] < dailyProduction) ? sharedData->orderQuantities[minOrder] : dailyProduction;

                printf("  %s is allocated to Order %s for the day. %d units to be produced. Remaining before production: %d\n", plantsArray[minPlant].name, orderArray[minOrder]->order_number, production, sharedData->orderQuantities[minOrder] - production);
                sharedData->days[minPlant] += 1;
                sharedData->produce[minPlant] += production;
                
                sharedData->orderQuantities[minOrder] -= production;

                availablePlants[minPlant] = 1;
                availableOrders[minOrder] = 1;
            }
            exit(EXIT_SUCCESS); 
        }
    }


    for (int i = 0; i < numDays; i++) {
        waitpid(pids[i], NULL, 0);
    }

    for (int i = 0; i < 3; i++) {
        plantsArray[i].days += sharedData->days[i];
        plantsArray[i].produce += sharedData->produce[i];
    }


    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, NULL);

    printReport(algorithm, fileName);
}


// helper function to assign orders to plants using FCFS
void assignOrdersToPlantsFCFS(Order* head, const char* fileName, const char* algorithm) {
    if (!head) return;

    int orderCount = countOrders(head);
    Order* orderArray[orderCount];
    int index = 0;
    for (Order* current = head; current != NULL; current = current->next) {
        if (canCompleteOrder(current, plantsArray[0].capacity, scheduling_period.start_date)) {
            orderArray[index++] = current;
        } else {
            rejectOrder(current);
        }
    }

    int numDays = numberOfDays;

    for (int day = 0; day < numDays; day++) {
        printf("Day %d:\n", day + 1);

        int availablePlants[3] = {0, 0, 0};  

        for (int i = 0; i < 3; i++) {
            if (!availablePlants[i]) {  
                for (int j = 0; j < orderCount; j++) {
                    if (orderArray[j]->quantity > 0) {  
                        int dailyProduction = plantsArray[i].capacity;
                        int production = (orderArray[j]->quantity < dailyProduction) ? orderArray[j]->quantity : dailyProduction;

                        printf("  %s is allocated to Order %s for the day. %d units to be produced. Remaining before production: %d\n",
                            plantsArray[i].name, orderArray[j]->order_number, production, orderArray[j]->quantity);

                        plantsArray[i].days += 1;
                        plantsArray[i].produce += production;

                        orderArray[j]->quantity -= production;
                        availablePlants[i] = 1;  
                        break;  
                    }
                }
            }
        }
        printf("\n");
    }
    printReport(algorithm, fileName);
}

// function to run scheduling algorithms
void runPLS(const char* input) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        close(fd[1]);

        char inputBuffer[256];
        read(fd[0], inputBuffer, sizeof(inputBuffer) - 1);
        close(fd[0]);

        inputBuffer[sizeof(inputBuffer) - 1] = '\0'; 

        char* start = strstr(inputBuffer, "runPLS");
        if (start) {
            start += strlen("runPLS");
            while (isspace(*start)) start++; 
        } else {
            start = inputBuffer;
        }

        char* token = strtok(start, "|");
        char* algorithm = token != NULL ? token : "";
        while (isspace(*algorithm)) algorithm++; 

        char* rest = strtok(NULL, "");
        if (rest) {
            while (isspace(*rest)) rest++; 

            const char* command = "printREPORT ";
            int command_len = strlen(command);
            if (strncmp(rest, command, command_len) == 0) {
                char* fileName = rest + command_len;
                while (isspace(*fileName)) fileName++;  

                if (strcasecmp(algorithm, "FCFS") == 0) {
                    assignOrdersToPlantsFCFS(head, fileName, algorithm);
                } else if (strcasecmp(algorithm, "SJF") == 0) {
                    assignOrdersToPlantsSJF(head, fileName, algorithm);
                } else {
                    fprintf(stderr, "Invalid algorithm: '%s'\ninput is expected in the format: '[no space] | printREPORT > [filename]'", algorithm);
                }
            } else {
                fprintf(stderr, "Command not recognized: '%s'\n", rest);
            }
        } else {
            fprintf(stderr, "Invalid input format. Expected '[no space] | printREPORT > [filename]'.\n");
        }

        exit(EXIT_SUCCESS); 
    } else { // Parent process
        close(fd[0]); 
        write(fd[1], input, strlen(input) + 1); 
        close(fd[1]);

        wait(NULL); 
    }
}

// helper function to compare two dates
int compareDates(const char* date1, const char* date2) {

    int year1, month1, day1;
    int year2, month2, day2;

    sscanf(date1, "%d-%d-%d", &year1, &month1, &day1);
    sscanf(date2, "%d-%d-%d", &year2, &month2, &day2);

    if (year1 < year2) return -1;
    if (year1 > year2) return 1;

    if (month1 < month2) return -1;
    if (month1 > month2) return 1;

    if (day1 < day2) return -1;
    if (day1 > day2) return 1;

    return 0; 
}

// main function
int main() {
    char command[256];
    printf("\n~~WELCOME TO PLS~~\n\n");
    while (1) {
        printf("Please enter:\n> ");
        fgets(command, sizeof(command), stdin); 

        size_t lenght = strlen(command);
        if (lenght > 0 && command[lenght - 1] == '\n') {
            command[lenght - 1] = '\0';
        }
        if (strncmp(command, "addPERIOD", 9) == 0) {
            char start_date[DATE], end_date[DATE];
            if (sscanf(command, "addPERIOD %s %s", start_date, end_date) == 2) {
                period(start_date, end_date);
                calculateDays(start_date, end_date);
            } 
        } else if (strncmp(command, "addORDER", 8) == 0) {
            char* line = strtok(command + 9, "\n");
            while (line != NULL) {
                char order_number[ORDER_ID], due_date[DATE], product_name[PRODUCT_NAME];
                int quantity;
                if (sscanf(line, "%s %s %d %s", order_number, due_date, &quantity, product_name) == 4) {

                    order(order_number, due_date, quantity, product_name);
                    numOrders = numOrders + 1;
                } 
                line = strtok(NULL, "\n");
            }
        } else if (strncmp(command, "addBATCH", 8) == 0) {
            char file_name[256];
            if (sscanf(command, "addBATCH %s", file_name) == 1) {
                FILE* file = fopen(file_name, "r");
                char order_number[ORDER_ID], due_date[100000], product_name[PRODUCT_NAME], command[9];
                int quantity;
                printf("Loading batch order from file %s\n", file_name);

                while (fscanf(file, "%s %s %s %d %s", command, order_number, due_date, &quantity, product_name) == 5) {

                    order(order_number, due_date, quantity, product_name);
                }
                fclose(file);
                printf("Batch file load complete.\n");
            }
        } else if (strncmp(command, "runPLS", 6) == 0) {
            runPLS(command);
        }  else if (strcmp(command, "exitPLS") == 0) {
            while (head != NULL) {
                Order* temp = head;
                head = head->next;
                free(temp);
            }
            printf("Exiting PLS.....\nWARNING: Order History Cleared\n");
            break;
        }else if(strcmp(command, "print") == 0){
            printOrderHistory();
        }
        else {
            fprintf(stderr, "WRONG COMMAND.\n");
        } 
    }
    return 0;
}


