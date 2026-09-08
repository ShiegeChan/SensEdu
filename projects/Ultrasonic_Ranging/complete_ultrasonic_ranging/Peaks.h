/*
 * Cross-correlation peak type shared by the ranging files.
 */

#ifndef PEAKS_H
#define PEAKS_H

#define MAX_PEAKS 1

struct Peak {
    float value;
    int location;
};

#endif