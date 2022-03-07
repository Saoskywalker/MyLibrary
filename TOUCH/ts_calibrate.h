#ifndef _TSCALIBRATE_H
#define _TSCALIBRATE_H
/*
 *  tslib/tests/ts_calibrate.h
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This file is placed under the GPL.  Please see the file
 * COPYING for more details.
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 *
 * functions used for calibrating 
 */
typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
	int a[7];
} ts_calibration;

extern ts_calibration touch_cal;

int perform_calibration(ts_calibration *cal);
int coords_get(ts_calibration *cal, int *xi, int *yi, int *xo, int *yo);

#endif /* _TSCALIBRATE_H */
