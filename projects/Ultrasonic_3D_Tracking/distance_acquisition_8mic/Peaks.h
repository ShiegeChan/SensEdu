/*
 * Cross-correlation peak type shared by the tracking files.
 */

#ifndef PEAKS_H
#define PEAKS_H

#define MAX_PEAKS 3

#define FULL_AVG_WIN 50


struct Peak {
    float value;
    int location;
};

#endif