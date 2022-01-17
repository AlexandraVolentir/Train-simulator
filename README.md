# Train-station-simulator

This project was created with the purpose of having an easy-to-use,
fast and interactive concurrent server, that will emulate the process of
”train scheduling” and even train moving. The server will have access to
an xml file that will give us information about the routes, number of trains,
arrival times, personal code numbers, working days, route distances, arrival 
times and etc., so that our train dispatcher can make a comparison
between the arrival predictions from the xml file and the in-time changing
schedule of arriving trains, and, as a result, make its estimations about
the time needed to wait for a train to arrive at a station.

We will implement a concurrent server which would fetch and send back
live information about ongoing routes taken by the trains. So, there would be
a smart concurrent server and two types of clients: the train drivers and the dispatcher.

## Train conductors
They will give concrete information about when they
have started their trains with a manual push, about when they arrive to a
station, so that the server can calculate its estimation on time arrivals and
everything else accordingly.

## Train dispatcher
It fetches from the server information about ongoing
delays, leavings and arrivals. It has a protocol through which he communicates
with the server.

## The server 
The server would receive information from the train’s movement and calculate if the 
train arrives differently than it’s prefixed schedule depending on the calculations
of the server based on the train driver’s received information. 
It will make and send its estimations about departures status, arrivals status, 
delays and arrival estimates to the train dispatcher. All the logic
and actual reasoning will take place in the server. It will fetch information about
the routes, stations, train particular information, planned arrival time for each
machine/client from an XML file. It will parse and compare pre-scheduled data
to the reality and send it to the dispatcher at the time intervals the client will ask for it.
