# todo

This project is incomplete and a WIP.

- Currently missing
	- testing

- Won't finish completely until project completion
	- Documentation and explanations

# c-http-server-template

This repository is meant to serve as a foundation for a HTTP server in C. Otherwise, it could be an educational reference.  

It could also be called an over-explained version of the documentation related to the headers that is used in this file.  

Or it could be used because reading documentation sucks.  

This server does nothing more but serve HTML. 

# Capabilties

The server created by this template, by default, is expecting a private, ([not public] (https://phoenixnap.com/kb/public-vs-private-ip-address)), IP address which listens on whatever port you decide.  

<br>

Accepts successful connections with a code 200 and returns "Hello Teto", otherwise 404.


# Intent

I created this repository because I felt like HTTP servers were big and scary, when in reality, the basic foundational concepts aren't hard to grasp at all. Most of the work, I believe, goes towards security and performance, making servers feel more complex than they really are at the core.  

Additionally, everything is additionally over-commented compared to my other works to promote quick understanding of the code.


# Q&A

One issue that I have with learning from open-source is understanding the "Why". So I'm going to try my best to explain things that are inherently not obvious in this section.  
 <br>

 - The calling convention for the [complete sample code for a server with HTTP given by the Microsoft documentations](https://learn.microsoft.com/en-us/windows/win32/winsock/complete-server-code) shows the calling convention for the main() function to use ```bash ___cdecl```. Why is there no specificied calling convention in this example?
 I believe that at this point, there's no more need to specify a calling convetion for functions, unless you're working with an application that MIXES calling conventions for its functions. It's confusing to newcomers, and calling conventions for functions are typically platform specific, so allowing the compiler to choose the calling convention for you allows this script to be compiled and ran on any system.

 - Why did you choose port 888 rather than a standard port like TCP 80?
 Because I wanted to. Technically, you could choose any port to listen to. But yes, port 80 is the standard for the TCP protocol.

 - 