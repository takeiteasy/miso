//
//  random.h
//  colony
//
//  Created by George Watson on 16/02/2023.
//

#ifndef random_h
#define random_h
#include <time.h>

typedef struct {
    unsigned int seed;
    int p1, p2;
    unsigned int buffer[17];
} Random;

Random NewRandom(unsigned int s);
unsigned int RandomBits(Random *r);
float RandomFloat(Random *r);
double RandomDouble(Random *r);
int RandomInt(Random *r, int max);
float RandomFloatRange(Random *r, float min, float max);
double RandomDoubleRange(Random *r, double min, double max);
int RandomIntRange(Random *r, int min, int max);

float Perlin(float x, float y, float z);

#endif /* random_h */
