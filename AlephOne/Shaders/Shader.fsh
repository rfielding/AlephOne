//
//  Shader.fsh
//  AlephOne
//
//  Created by Robert Fielding on 10/14/11.
//  Copyright 2011 Rob Fielding Software.
//

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}
