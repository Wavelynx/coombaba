# Coombaba 

## How To User

Simply include the header only library include/wvapi/step.hpp.

`step.hpp` automatically includes the parser and turns whatever file
you have into a binary executable. It comes with some command lines
out of the box. Bellow is an example of a feature file built using
our framework.

./client.spec --file=./features/client-consumer-remote.feature

The client-consumer-remote.feature will look something like:

    Feature: I have a test client

        Scenario: I have a cam-agent which connects

            Given I have a cam-agent {remote_agent}
            Given cam-agent {remote_agent} - connects to mqtt service with config {/home/root/confs/remote-consumer.json}
            Then cam-agent {remote_agent} runs forever

This creates a connection with the mqtt broker as a consumer. We then
fire up the same binary with different feature that acts as the publisher:

./build/client.spec --file=./features/client-publisher-remote.feature

    Feature: I have a test client

        Scenario: I have a cam-agent which connects

            Given I have a cam-agent {remote_agent}
            Given cam-agent {remote_agent} - connects to mqtt service with config {/home/root/confs/remote-publisher.json}
            Then cam-agent {remote_agent} publishes {hello}

You can see that we have fine grain control over the behaviour of the 
program without having to recompile.

This paradigm allows us to not only run very specific tests that makes
sense on the entire executable, but also run tests on the libraries,
header files and modules that make up the end executable and a business
friendly manner.

### How To Create Test Executables

We follow the paradigm of creating step definitions in the include folder. 
Everything we do is a header file. When building a new step executable create
a file with the ending .spec.cpp. For instance, if we wanted to create a test
executable of the auth.hpp file, we would create a file called:

./include/wvapi/auth.spec.cpp

In this file we would then include the following:

    #include <wvapi/step.hpp>

This header file creates the executable and all the macros necessary for building.
This header file requires the availability of two libraries... boost/sml.hpp and
boost regex.

### How to Create Step Functions

To create a step function you have 3 Macros:

    - GIVEN
    - WHEN
    - THEN

There is no difference in functionality of the GIVEN, WHEN and THEN. It's only
a matter of what makes the test sound good.

    GIVEN("I authenticate with server {}") {
        auto ip = STEP_VALUES[0];
        ... code ...
        if (pass) return true;
        return false;
    }

You'll notice that you have a reserved key token `{}` which indicates a parameter.
All `{}` parameters are interpreted as strings and mapped to an internal vector
called `STEP_VALUES`. To access the parameter value in this function, you just
use an index. The index is dictated by the parameter order as you can see in this
example. If we use this step in a feature file, it will look like:

    GIVEN I authenticate with server {123.123.123.123}

That STEP function will return the string "123.123.123.123" in STEP_VALUES[0].
Lastly all STEP functions must return a bool value indicating the success of the
message.
