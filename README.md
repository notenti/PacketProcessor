# Target Platform
Compiled on an Ubuntu 16.04 LTS system using C++ 14 with g++ version 5.4.0.

# Code Description
The primary function of the code is to provide a compression service. The service takes in a payload in the form of a string or series of strings and returns the compressed version of said payload. The compression service will reject any payload with an uppercase letter or number. An example of the service:

#### Compression Service

dog => dog

dooog => d3og

doooooooooog => d10og

Dog => [Error]

dog12 => [Error]

it is warm out today => it is warm out today

it is warm out today!!!!! => it is warm out today5!

it        is => it8 is

In short, the service will take any repeating character, save uppercase characters and numbers, and compress it if necessary. The service will also compress spaces, so sometimes the compressed output will look a little odd. In the following example I've denoted spaces with hyphens (-):

i-love-spaces----- => i-love-spaces5-

As you can see, the compressed output will be terminated by a 5 and then a space. When reading this it looks odd, but it's the correct output for the service.

The code also supports some other queries. Those are as follows:

#### Ping

The **Ping** service does nothing more than checks whether the socket communication is operational. If the service is working, the client will receive a packet to indicate such. If the service is not working, or if there was an issue with the ping packet, the client will be also be notified of that.

#### Get Stats

The **Get Stats** service will report back certain statistics about the current instance. It will return to the client the total bytes received, the total bytes sent, and the compression ratio. The compression ratio is derived by taking the total number of bytes spat out the compression algorithm and dividing that by the total number of bytes fed into the compression algorithm. A compression ratio of 90 indicates that the service hasn't been able to compress much of the data, while a compression ratio of 10 means that it has.

#### Reset Stats

The **Reset Stats** service will zero out all of the counted statistics for the specific instance.

## Error Reporting

The code has some support for error reporting. Every packet that comes into the server is checked for general health. These checks are: whether the *magic number* denoted by the instructions matches the one received, whether the request code is anything besides 1, 2, 3, or 4, and whether the payload exceeds the maximum payload length (12KB). Further, any payload that is to be fed through the compression service will also be checked for uppercase letter and numbers. If the payload fails any of these checks the client will be properly notified.

I added two eror codes that were not specified in the assignment. These error codes are:

33: **Magic Number Error** - When the Magic Number received at the server is not the one given in the assignment and error will be thrown back to the client.
34: **Compression Error** - When the compression service fails to compression the string due to uppercase letters or number an error will be thrown back to the client.

If at any point the server receives a payload consisting of a single '#' character, it will safely terminate the connection.
