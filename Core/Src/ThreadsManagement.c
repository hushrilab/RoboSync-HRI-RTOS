#include "kernel.h"
#include <jansson.h>
#include <stdio.h>

typedef struct {
    char name[50];
    char type[20];
    char address_or_pin[10];
} SensorConfig;

typedef struct {
    char name[50];
    char type[20];
    char pin[10];
} ActuatorConfig;

typedef struct {
    char name[50];
    char action[50]; // Actuator to trigger
} BehaviorConfig;

typedef struct {
    char name[50];
    char path[255];
    void *handle; // Handle to the loaded module
} AlgorithmConfig;

#define MAX_ACTUATORS 10
#define MAX_BEHAVIORS 10
#define MAX_ALGORITHMS 10
#define MAX_SENSORS 10
SensorConfig sensorConfigs[MAX_SENSORS];
ActuatorConfig actuatorConfigs[MAX_ACTUATORS];
BehaviorConfig behaviorConfigs[MAX_BEHAVIORS];
AlgorithmConfig algorithmConfigs[MAX_ALGORITHMS];

int num_sensors = 0;
int num_actuators = 0;
int num_behaviors = 0;
int num_algorithms = 0;

void sensor_thread_handler(void *args) {
    SensorConfig *sensor = (SensorConfig *)args;
    while(1) {
        if(strcmp(sensor->type, "I2C") == 0) {
            printf("Reading from I2C sensor: %s at address %s\n", sensor->name, sensor->address_or_pin);
        } else if(strcmp(sensor->type, "GPIO") == 0) {
            printf("Reading from GPIO sensor: %s at pin %s\n", sensor->name, sensor->address_or_pin);
        }
        osYield();
    }
}
void actuator_thread_handler(void *args) {
    ActuatorConfig *actuator = (ActuatorConfig *)args;
    while(1) {
        if(strcmp(actuator->type, "PWM") == 0) {
            printf("Activating PWM actuator: %s at pin %s\n", actuator->name, actuator->pin);
        }
        // Add more actuator types as needed
        osYield();
    }
}

void behavior_thread_handler(void *args) {
    BehaviorConfig *behavior = (BehaviorConfig *)args;
    while(1) {
        if(strcmp(behavior->name, "temperature_check") == 0) {
            printf("Checking temperature and triggering action: %s\n", behavior->action);
        }
        // Add more behavior checks as needed
        osYield();
    }
}

void algorithm_thread_handler(void *args) {
    AlgorithmConfig *algorithm = (AlgorithmConfig *)args;
    // Load the module using dlopen (ensure this is supported on your platform)
    algorithm->handle = dlopen(algorithm->path, RTLD_LAZY);
    if (!algorithm->handle) {
        fprintf(stderr, "Error loading algorithm: %s\n", dlerror());
        return;
    }

    void (*algorithm_func)() = dlsym(algorithm->handle, "run");
    if (!algorithm_func) {
        fprintf(stderr, "Error finding 'run' function in algorithm: %s\n", dlerror());
        return;
    }

    while(1) {
        algorithm_func();
        osYield();
    }
}


void parse_config_file(const char *filename) {
    json_t *root;
    json_error_t error;
    size_t index;
    json_t *value;

    root = json_load_file(filename, 0, &error);
    if (!root) {
        fprintf(stderr, "Error: %s\n", error.text);
        return;
    }

    // Parse sensors
    json_t *sensors = json_object_get(root, "sensors");
    json_array_foreach(sensors, index, value) {
        strcpy(sensorConfigs[num_sensors].name, json_string_value(json_object_get(value, "name")));
        strcpy(sensorConfigs[num_sensors].type, json_string_value(json_object_get(value, "type")));
        strcpy(sensorConfigs[num_sensors].address, json_string_value(json_object_get(value, "address")));
        num_sensors++;
    }

    // Parse actuators
    json_t *actuators = json_object_get(root, "actuators");
    json_array_foreach(actuators, index, value) {
        strcpy(actuatorConfigs[num_actuators].name, json_string_value(json_object_get(value, "name")));
        strcpy(actuatorConfigs[num_actuators].type, json_string_value(json_object_get(value, "type")));
        strcpy(actuatorConfigs[num_actuators].pin, json_string_value(json_object_get(value, "pin")));
        num_actuators++;
    }

    // Parse behaviors
    json_t *behaviors = json_object_get(root, "behaviors");
    json_array_foreach(behaviors, index, value) {
        strcpy(behaviorConfigs[num_behaviors].name, json_string_value(json_object_get(value, "name")));
        strcpy(behaviorConfigs[num_behaviors].action, json_string_value(json_object_get(value, "action")));
        num_behaviors++;
    }

    // Parse algorithms
    json_t *algorithms = json_object_get(root, "algorithms");
    json_array_foreach(algorithms, index, value) {
        strcpy(algorithmConfigs[num_algorithms].name, json_string_value(json_object_get(value, "name")));
        strcpy(algorithmConfigs[num_algorithms].path, json_string_value(json_object_get(value, "path")));
        num_algorithms++;
    }

    json_decref(root);  // Cleaning up the JSON structures
}

int main() {
    osKernelInit();

    parse_config_file("config.json");

    for(int i = 0; i < num_sensors; i++) {
        osCreateThread(sensor_thread_handler, &sensorConfigs[i]);
    }

    for(int i = 0; i < num_actuators; i++) {
        osCreateThread(actuator_thread_handler, &actuatorConfigs[i]);
    }

    for(int i = 0; i < num_behaviors; i++) {
        osCreateThread(behavior_thread_handler, &behaviorConfigs[i]);
    }

    for(int i = 0; i < num_algorithms; i++) {
        osCreateThread(algorithm_thread_handler, &algorithmConfigs[i]);
    }

    osKernelStart();

    return 0;
}
