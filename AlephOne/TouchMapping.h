//
//  TouchMapping.h
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Rob Fielding Software.
//

int TouchMapping_mapFinger(void* touch);
void TouchMapping_unmapFinger(void* touch);

int TouchMapping_mapFinger2(void* touch);
void TouchMapping_unmapFinger2(void* touch);

int TouchMapping_finger2FromFinger1(int finger);