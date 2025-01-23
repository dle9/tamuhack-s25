current issues:
- in the bluetooth, every loop we delay 2000 ms between broadcasts. we don't necessarily have to wait, but we do want to make sure we have data to send
- deciding what is a long / short: my vote is to track milliseconds between changes. if the state of the audio changes in under .5 seconds, it's short. if it's longer than that, it's long. timing can be adjusted however we want
- transmitting the right amount of data (< 250 bytes) at a time, while not pausing/impacting data collection or reception
