//
//  Shader.vsh
//  AlephOne
//
//  Created by Robert Fielding on 10/14/11.
//  Copyright 2011 Rob Fielding Software.
//

attribute vec4 position;
attribute vec4 color;

varying vec4 colorVarying;

uniform float translate;

void main()
{
    gl_Position = position;
    gl_Position.y += sin(translate) / 2.0;

    colorVarying = color;
}
