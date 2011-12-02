//
//  TouchMapping.h
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct Fretless_context;

int TouchMapping_mapFinger(struct Fretless_context* ctxp, void* touch);
void TouchMapping_unmapFinger(struct Fretless_context* ctxp, void* touch);

int TouchMapping_mapFinger2(struct Fretless_context* ctxp, void* touch);
void TouchMapping_unmapFinger2(struct Fretless_context* ctxp, void* touch);
