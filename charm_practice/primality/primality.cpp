#include "Primality.decl.h"
#include <vector>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <iostream>

class Main : public CBase_Main {
private:
  std::vector<std::pair<int, bool>> results;
  int remaining;

public:
  Main(CkArgMsg* m) {
    if (m->argc < 2) {
      CkPrintf("Usage: ./charmrun +pN primality K\n");
      CkExit();
    }
    int K = atoi(m->argv[1]);

    results.resize(K, {0, false});
    remaining = K;

    srand(time(NULL));

    for (int i = 0; i < K; i++) {
      int num = abs(rand()) % 100 + 1;
      results[i].first = num;

      // Fire a worker chare with index + number
      CProxy_checkPrimality::ckNew(i, num, thisProxy);
    }
  }

  void checkDone(int index, int isPrimeResult) {
    results[index].second = (isPrimeResult == 1);
    remaining--;

    if (remaining == 0) {
      CkPrintf("Results:\n");
      for (size_t i = 0; i < results.size(); i++) {
        CkPrintf("%d : %s\n", results[i].first,
                 results[i].second ? "Prime" : "Not Prime");
      }
      CkExit();
    }
  }
};

class checkPrimality : public CBase_checkPrimality {
public:
  checkPrimality(int index, int number, CProxy_Main m) {
    int res = isPrime(number);
    m.checkDone(index, res);
  }

private:
  int isPrime(const long number) {
    if (number <= 1) return 0;
    if (number == 2) return 1;
    if (number % 2 == 0) return 0;
    for (int i = 3; i * i <= number; i += 2) {
        if (number % i == 0) return 0;
    }
    return 1;
}
};

#include "Primality.def.h"
