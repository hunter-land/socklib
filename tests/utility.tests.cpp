#include "steps.hpp"
#include "testing.hpp"
#include <ostream>
#include <chrono>

/**
 *	This file contains all utility tests for functionality of sockets.
 *
 */

void ReadReadyTimesOutCorrectly(std::ostream& log, sks::domain d, sks::type t) {
	///Prerequisites to this test (throw results unable to meet requirement)///
	GivenTheSystemSupports(log, d, t);

	///Test variables///
	std::chrono::milliseconds timeoutGrace(10); //Allow 10ms extra for timeout
	std::vector<std::chrono::milliseconds> timeouts = { std::chrono::milliseconds(0), std::chrono::milliseconds(100) };

	//For timeouts of { 0ms, 100ms }
	//Two connected sockets of d and t
	//Both should have readReady == false when given T to time out
	//Have one send a message, check readReady again with timeout T

	//Regardless of a result, if the function takes too long (T + 10ms) fail the test

	auto sockets = GetRelatedSockets(log, d, t);
	sks::socket& sockA = sockets.first.socket;
	sks::socket& sockB = sockets.second.socket;
	std::chrono::time_point<std::chrono::system_clock> preTimeoutTime, postTimeoutTime;
	std::chrono::milliseconds timeDiff;
	bool tooQuick = false, tooSlow = false;

	for (std::chrono::milliseconds timeoutTime : timeouts) {
		std::chrono::milliseconds timeMax = timeoutGrace + timeoutTime;
		log << "Checking with a timeout of " << timeoutTime.count() << "ms" << std::endl;
		log << "Checking should not take more than " << timeMax.count() << "ms" << std::endl;
		if (t == sks::stream || t == sks::seq) {
			//Connected types

			//First check without any data to read
			{
				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockA.readReady(timeoutTime) == false, "socket A returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				testing::assertTrue(
					!tooQuick && !tooSlow,
					std::string(tooQuick ? "readReady() returned prematurely" : "readReady() took longer than timeout to return") + "\n"
					"Expected [ ~" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);

				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockB.readReady(timeoutTime) == false, "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				testing::assertTrue(
					!tooQuick && !tooSlow,
					std::string(tooQuick ? "readReady() returned prematurely" : "readReady() took longer than timeout to return") + "\n"
					"Expected [ ~" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);
			}

			//Now send data to just one socket and check both again
			//Note: If there is data ready, returning early is allowed, but it should wait until the timeout finishes if there is not data
			sockB.send( {'A'} ); //Send a byte
			//A tiny sleep to allow the byte to flush and get received in the buffer (to help with flakiness)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			//Check again, results should be different for sockA
			{
				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockA.readReady(timeoutTime) == true, "socket A returned false for readable when there was data");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				testing::assertTrue(
					timeDiff < timeMax,
					"readReady() took longer than timeout to return\n"
					"Expected [ <" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);

				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockB.readReady(timeoutTime) == false, "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				testing::assertTrue(
					!tooQuick && !tooSlow,
					std::string(tooQuick ? "readReady() returned prematurely" : "readReady() took longer than timeout to return") + "\n"
					"Expected [ ~" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);
			}

			//Read the byte so the next timeout check starts without data
			sockA.receive();
		} else {
			//non-connected types should always return true when bound

			//First check without any data to read
			{
				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockA.readReady(timeoutTime) == false, "socket A returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				testing::assertTrue(
					!tooQuick && !tooSlow,
					std::string(tooQuick ? "readReady() returned prematurely" : "readReady() took longer than timeout to return") + "\n"
					"Expected [ ~" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);

				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockB.readReady(timeoutTime) == false, "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				testing::assertTrue(
					!tooQuick && !tooSlow,
					std::string(tooQuick ? "readReady() returned prematurely" : "readReady() took longer than timeout to return") + "\n"
					"Expected [ ~" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);
			}

			//Now send data to just one socket and check both again
			//Note: If there is data ready, returning early is allowed, but it should wait until the timeout finishes if there is not data
			sockB.send( {'A'}, sockA.localAddress() ); //Send a byte
			//A tiny sleep to allow the byte to flush and get received in the buffer (to help with flakiness)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			//Check again, results should be different for sockA
			{
				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockA.readReady(timeoutTime) == true, "socket A returned false for readable when there was data");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				testing::assertTrue(
					timeDiff < timeMax,
					"readReady() took longer than timeout to return\n"
					"Expected [ <" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);

				preTimeoutTime = std::chrono::system_clock::now();
				testing::assertTrue(sockB.readReady(timeoutTime) == false, "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				testing::assertTrue(
					!tooQuick && !tooSlow,
					std::string(tooQuick ? "readReady() returned prematurely" : "readReady() took longer than timeout to return") + "\n"
					"Expected [ ~" + std::to_string(timeMax.count()) + "ms ]\n"
					"Actual   [ " + std::to_string(timeDiff.count()) + "ms ]"
				);
			}

			//Read the byte so the next timeout check starts without data
			sockA.receive();
		}
	}
}
