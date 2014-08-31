WebVideo
========

A remote camera monitoring system implemented by v4l2 and ffmpeg. ARM development board Mini440 serves as monitor with USB camera mounting on and a PC with linux system displays the video acquisited from monitor area.

PCShower:  Compiled for arch X86 to decode ffmpeg and display the video in X11 window.

WebServer: Compiled for arch ARM to encode ffmpeg and send data to PCShower by Socket.


Modified @[sunkwei](http://blog.csdn.net/sunkwei/article/details/6530343)
