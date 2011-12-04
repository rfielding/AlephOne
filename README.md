AlephOne
========

This project is lessons learned from Mugician and Geo Synth.  I have no idea if this code will literally be released as an instrument.  But it's a lab rat designed to produce clean code modules that are very isolated and heavily tested.  As such, I will be resisting any attempts to let the modules start to build up unnecesary dependencies and complexity.

After only a few weeks of effort, it is better than Geo in a lot of ways (almost entirely in the Fretless module).

http://www.youtube.com/watch?v=ZGSZBsxYMfI

Main Components
---------------

As far as conventions being followed, I don't really consider any code that is not written in pure C to be all that portable or reuseable for these purposes.  These are the main components so far, listed in the order in which it is most important that they stay portable:

* Fretless -- A totally isolated MIDI library. This library is the essence of what makes Mugician and Geo special, and would allow almost anybody to painlessly write such an app (so long as it talks to MIDI of course).  It is designed as a portable C buffer generator of MIDI data.  All dependencies are injected, and all types are built-in types.  

All of this is necessary because one goal of this library is to standardize how Fretless MIDI is done; to fix the MIDI standard for touch-screens essentially.  SampleWiz and ThumbJam recognize this MIDI, and Geo emits it.  The plan is to get this code very solid, and see if we can get many synths to understand this code, and for many controllers to emit it.  That will never work if there are application specific dependencies abound in the code.  This is designed so that almost any app that is using MIDI can start here, even if they have their own plans for all of the other modules.

* PitchHandler -- An isolated pitch handling library, that is currently handling the main coordinate system problems as well.  One of the main problems I had with Mugician and Geo is that coordinate systems not done right, spread complexity absolutely everywhere else throughout the code.  This problem starts with the fact that basic things like touches and OpenGL windows have different coordinate system conventions by default.  We immediately get out of coordinates to pitches as soon as that is possible.  

Where coordinates are necessary, we ensure that this is done so that the coordinate system that will be used by the OpenGL code later on will be identical, so that there is no proliferation of equations throughout the code trying to compensate for the inconsistency.  

This module may eventually split as it ties together coordinate systems too tightly with pitch handling proper.  This module will provide the metadata that the OpenGL surfaces will need to actually render.  This means that once the moveable frets and snapping rules are set, OpenGL should be able to run some iterators in this library to figure out how to draw the surface and how to show the touches to the user.

* TouchMaping -- An isolated library for mapping pointers to some kind of touch object to finger identifiers.   

* ControlRegistry -- A library that is being planned.  When bringing in many modules from different code that are isolated, by definition they will not be importing code and conventions from a common location.  But in places where knobs, sliders, and switches are used we can collect these points into a set of controls.  As an example, most libraries will have a get/set function pair for some integer and floating point values.  The library as imported won't have all the information to make a control, but the registry could map the pair up to a function that returns a display name and a string value for the current get value.  In this way, components that simply provide functionality can stay out of the business of exposing parameters in an app specific way.  This should be the job of the application that's assembling the components for its own purposes.

* VertexObjectBuilder -- An isolated library that makes it easy to generate OpenGL objects ahead of time (ie: lines, triangle strips).  It has no dependencies on anything, including no dependencies on OpenGL itself.

* GenericTouchHandling -- This library has a few dependencies on our C libraries, but still doesn't make any references into any ObjectiveC or Apple libraries.  It is the portable parts of code that handle touches going up and down, only assuming that we got some kind of pointer for the touch.

* CoreMIDIRenderer -- The Fretless MIDI API invokes function pointers like putchar/flush to generate MIDI packets.  Either a synthesizer, or a proxy for a synthesizer can implement this interface.  CoreMIDIRenderer is just such a proxy, and only depends on Fretless and CoreMIDI.  The interface is C, but the implementation is Objective-C, and is the first thing in this list that won't be useable on Android for sure (though it's a small piece of code that would have an equivalent there.) It is the simplest implementation of an object that looks like a synthesizer to Fretless.  If we embed an internal sound engine, it should have a similar interface.  Doing things this way yields an incredible number of advantages.  The primary advantage is that it becomes easy to both test MIDI synths locally, and allow for synth engines to be per-patch (sampling based versus subtractive versus just sending the messages on to somewhere else).  Most people wanting to quickly build an app will use this along with Fretless to get the app up and running.


 
