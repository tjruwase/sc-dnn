#include "Utils.h"
#include <iostream>
#include <vector>
#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;

//long numOnlineCPUs = sysconf(_SC_NPROCESSORS_ONLN);

void SetTrainingThreadAffinity(int threadNum)
{
  /*
  cpu_set_t cpuset;
  
  CPU_ZERO(&cpuset);
  CPU_SET((threadNum % numOnlineCPUs), &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  */
}

static struct option long_options[] = {
  {"samples", required_argument, 0, 's'},
  {"threads", required_argument, 0, 't'},
  {"workers", required_argument, 0, 'w'},
  {"model", required_argument, 0, 'm'},
  {"partition", no_argument, 0, 'p'},
  {"nodeltaweight", no_argument, 0, 'd'},
  {"startlayer", required_argument, 0, 'l'},
  {"classify", no_argument, 0, 'c'},
  {"affinity", no_argument, 0, 'a'},
  {"forwardsparsity", required_argument, 0, 'f'},
  {"backwardsparsity", required_argument, 0, 'b'},
  {"deltacomputesparsity", required_argument, 0, 'e'},
  {"weightupdatesparsity", required_argument, 0, 'u'},
  {"sparsekernels", required_argument, 0, 'k'},
  {0, 0, 0, 0}
};

void SetCanonicalConfig(int argc, char* argv[], CanonicalConfig& config)
{
  while (1) {
    int long_option_index = 0;
    int c = getopt_long_only(argc, argv, "", long_options, &long_option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
    case 's':
      config._sampleCount = atoi(optarg);
      break;
    case 't':
      config._threadCount = atoi(optarg);
      break;
    case 'w':
      config._workerCount = atoi(optarg);
      break;
    case 'm':
      config._modelType = ProcessModelParam(optarg);
      break;
    case 'p':
      config._replicatedOutputLayer = false;
      break;
    case 'd':
      config._deltaWeightOpt = false;
      break;
    case 'l':
      {
	int layer = atoi(optarg);
	config._startLayer = (layer < ModelLayerCount[(int)config._modelType]) ? layer : DEFAULT_START_LAYER;
      }
      break;
    case 'c':
      config._training = false;
      break;
    case 'a':
      config._affinity = true;
      break;
    case 'f':
      config._forwardSparsity = atoi(optarg);
      break;
    case 'b':
      config._backwardSparsity = atoi(optarg);
      break;
    case 'e':
      config._deltaComputeSparsity = atoi(optarg);
      break;
    case 'u':
      config._weightUpdateSparsity = atoi(optarg);
      break;
    case 'k':
        config._sparseKernelVersion = atoi(optarg);
      break;
    default:
      printf("%c: **No Match**\n", c);
    }
  }
}

static void* s_DNNModelThreadForward(void *arg)
{
  DNNModelThreadForward((ThreadLayerState*) arg);
  return NULL;
}

static void* s_DNNModelThreadBackward(void *arg)
{
  DNNModelThreadBackward((ThreadLayerState*) arg);
  return NULL;
}

static void* s_DNNModelThreadWeightUpdate(void *arg)
{
  if (G_DELTA_WEIGHT_OPT) {
    DNNModelThreadDeltaWeightUpdate((ThreadLayerState*) arg);
  }
  else {
    DNNModelThreadWeightUpdate((ThreadLayerState*) arg);
  }
  return NULL;
}

void DoModelCompute(int numThreads, ThreadLayerState *tl, DNNPass dp)
{
  pthread_t* helperThreads = new pthread_t [numThreads];
  int* errors = new int[numThreads];

  for (int i = 0; i < numThreads; i++) {
    
    if (dp == DNN_FORWARD)
      errors[i] = pthread_create(&helperThreads[i], NULL, &s_DNNModelThreadForward, (void*) (tl+i));
    else if (dp == DNN_BACKWARD)
      errors[i] = pthread_create(&helperThreads[i], NULL, &s_DNNModelThreadBackward, (void*) (tl+i));
    else 
      errors[i] = pthread_create(&helperThreads[i], NULL, &s_DNNModelThreadWeightUpdate, (void*) (tl+i));
  }
  
  for (int i = 0; i < numThreads; i++) {
    if (errors[i] == 0) {
      errors[i] = pthread_join(helperThreads[i], NULL);
    }
  }
}
