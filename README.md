# JustBreath <br>

### In PlatformIO, add library for bme280 <br>
Search BME280 mbed, author "Paul Staron" <br>
### BME280 repository <br>
https://os.mbed.com/users/star297/code/BME280/?utm_source=platformio&utm_medium=piohome

### Sample time:
Our while loop waits for 500ms in every iteration. That means we read the data from sensor every 0.5 sec. So each of the following messages represents 0.5 seconds elapsed.

### Scenario 1: when the system first starts, no inhaling or exhaling, humidity is around 59-61% with slightly decreasing or no-changing. 
* restart...
* humidity: 61  | temperature: 24  | pressure: 101737 
* humidity: 61  | temperature: 24  | pressure: 101734 
* humidity: 60  | temperature: 24  | pressure: 101737 
* humidity: 60  | temperature: 24  | pressure: 101738 
* humidity: 60  | temperature: 24  | pressure: 101736 
* humidity: 60  | temperature: 24  | pressure: 101734 
* humidity: 60  | temperature: 24  | pressure: 101737 
* humidity: 59  | temperature: 24  | pressure: 101736 
* humidity: 59  | temperature: 24  | pressure: 101737 
* humidity: 59  | temperature: 24  | pressure: 101736
* humidity: 59  | temperature: 24  | pressure: 101736 
* humidity: 59  | temperature: 24  | pressure: 101735 
* humidity: 59  | temperature: 24  | pressure: 101735 
* humidity: 59  | temperature: 24  | pressure: 101735 
* humidity: 59  | temperature: 24  | pressure: 101739 
* humidity: 59  | temperature: 24  | pressure: 101737 
* humidity: 59  | temperature: 24  | pressure: 101735 
* humidity: 59  | temperature: 24  | pressure: 101736 
* humidity: 59  | temperature: 24  | pressure: 101738 
* humidity: 59  | temperature: 24  | pressure: 101739 
* end of a cycle


### Scenario 2: when an individual is breathing,  there must be some increasing in the humidity values. The increasing periods are highlighted below. These are caused by the exhalations. When inhaling, the humidity will slightly decrease. And, when exhaling, the humidity never goes down.

* restart...
* humidity: 64  | temperature: 24  | pressure: 101729 
* humidity: 65  | temperature: 24  | pressure: 101733 
* humidity: 65  | temperature: 24  | pressure: 101731 
* humidity: 64  | temperature: 24  | pressure: 101732 
* humidity: 64  | temperature: 24  | pressure: 101732 
* humidity: 64  | temperature: 24  | pressure: 101733 
* humidity: 63  | temperature: 24  | pressure: 101730 
* humidity: 65  | temperature: 24  | pressure: 101733 
* humidity: 66  | temperature: 24  | pressure: 101731 
* humidity: 66  | temperature: 24  | pressure: 101730 
* humidity: 65  | temperature: 24  | pressure: 101732 
* humidity: 65  | temperature: 24  | pressure: 101730 
* humidity: 64  | temperature: 24  | pressure: 101730 
* humidity: 64  | temperature: 24  | pressure: 101731 
* humidity: 64  | temperature: 24  | pressure: 101729 
* humidity: 64  | temperature: 24  | pressure: 101732 
* humidity: 64  | temperature: 24  | pressure: 101732 
* humidity: 66  | temperature: 25  | pressure: 101731 
* humidity: 67  | temperature: 25  | pressure: 101732 
* humidity: 68  | temperature: 25  | pressure: 101732 
* end of a cycle
* restart...


### Scenario 3: when an individual stops breathing, the humidity values start falling back to 59-61%, with possibly slight fluctuation. But the increase will never greater than that when exhaling.
restart...
* humidity: 67  | temperature: 25  | pressure: 101726 
* humidity: 68  | temperature: 25  | pressure: 101724 
* humidity: 67  | temperature: 25  | pressure: 101724 
* humidity: 66  | temperature: 25  | pressure: 101722 
* humidity: 65  | temperature: 25  | pressure: 101724 
* humidity: 65  | temperature: 25  | pressure: 101726 
* humidity: 65  | temperature: 25  | pressure: 101722 
* humidity: 65  | temperature: 25  | pressure: 101723 
* humidity: 65  | temperature: 25  | pressure: 101723 
* humidity: 65  | temperature: 25  | pressure: 101722 
* humidity: 65  | temperature: 25  | pressure: 101723 
* humidity: 65  | temperature: 25  | pressure: 101724 
* humidity: 65  | temperature: 25  | pressure: 101723 
* humidity: 65  | temperature: 25  | pressure: 101720 
* humidity: 65  | temperature: 25  | pressure: 101722 
* humidity: 65  | temperature: 25  | pressure: 101722 
* humidity: 65  | temperature: 25  | pressure: 101723 
* humidity: 64  | temperature: 25  | pressure: 101722 
* humidity: 64  | temperature: 25  | pressure: 101723 
* humidity: 63  | temperature: 25  | pressure: 101719 
* end of a cycle
* restart...



### Algo1 – Increasing Detection:
From the observations, we know when we are breathing, there must be inhalations and exhalations. So the humidity goes down and up periodically. And the <strong> exhalation causes greater impact on the humidity</strong> than the inhalation do. So we decide determining whether there is breath by spotting out the exhalation. 

We know that when the value goes up for few seconds and the difference accumulates to a certain value, there must be an exhalation. And we know that when we are not breathing, there is no any significant increasing on the humidity or there would be slow decrease or no changing. So this algorithm is to find the <strong>contiguous increasing periods</strong> on the humidity, or say the exhalations.

|       |Increasing    | Decreasing   |
|:---   |:---          |:---          |
|Inhale |N/A           |-0.0 ~ -3.0   |
|Exhale |+3.0 ~ +6.0   |N/A           |
|Stop   |+0.0 ~ +1.0   |Back to normal|

1.	Start the measurement cycle
2.	Collect the humidity data into a sample buffer
3.	Find out whether it is increasing or decreasing by compare current and previous sample
4.	If in an increasing period, calculate <strong> the deltas (sample[curr]-sample[prev])</strong>
    Add them up, because we have to capture the <strong>accumulated deltas</strong>
    Typically, when they sum to 3.0 increasing, means that the person is exhaling
    Then reset the timer, because there is a breathing
5.	If in a decreasing period, we zero the accumulated increasing
    (because we use it to spot out the exhalations, there is no exhalation in decreasing period)
6.	Increment the timer, when it goes to 10s, means there is no exhalation (breathing) for 10s.
7.	Stop the measurement cycle


The highlighted messages are the increasing period.

* restart...
* time: 0 |  humidity: 64  | temperature: 25  | pressure: 101759 -> exhaling...
* time: 1 |  humidity: 66  | temperature: 25  | pressure: 101760 -> exhaling...
* time: 1 |  humidity: 66  | temperature: 25  | pressure: 101760 
* time: 2 |  humidity: 65  | temperature: 25  | pressure: 101759 
* time: 3 |  humidity: 64  | temperature: 25  | pressure: 101761 
* time: 4 |  humidity: 67  | temperature: 25  | pressure: 101758 -> exhaling...
* time: 1 |  humidity: 69  | temperature: 25  | pressure: 101758 -> exhaling...
* time: 1 |  humidity: 70  | temperature: 25  | pressure: 101760 -> exhaling...
* time: 1 |  humidity: 69  | temperature: 25  | pressure: 101759 
* time: 2 |  humidity: 68  | temperature: 25  | pressure: 101758 
* time: 3 |  humidity: 66  | temperature: 25  | pressure: 101761 
* time: 4 |  humidity: 66  | temperature: 25  | pressure: 101759 
* time: 5 |  humidity: 67  | temperature: 25  | pressure: 101759 
* time: 6 |  humidity: 69  | temperature: 25  | pressure: 101760 -> exhaling...
* time: 1 |  humidity: 70  | temperature: 25  | pressure: 101761 -> exhaling...
* time: 1 |  humidity: 70  | temperature: 25  | pressure: 101759 -> exhaling...
* time: 1 |  humidity: 70  | temperature: 25  | pressure: 101762 
* time: 2 |  humidity: 69  | temperature: 25  | pressure: 101760 
* time: 3 |  humidity: 67  | temperature: 25  | pressure: 101758 
* time: 4 |  humidity: 66  | temperature: 25  | pressure: 101759 
* time: 5 |  humidity: 68  | temperature: 25  | pressure: 101759 
* time: 6 |  humidity: 70  | temperature: 25  | pressure: 101761 -> exhaling...
* time: 1 |  humidity: 71  | temperature: 25  | pressure: 101761 -> exhaling...
* time: 1 |  humidity: 72  | temperature: 25  | pressure: 101764 -> exhaling...
* time: 1 |  humidity: 71  | temperature: 25  | pressure: 101763 
* time: 2 |  humidity: 69  | temperature: 25  | pressure: 101759 
* time: 3 |  humidity: 68  | temperature: 25  | pressure: 101760 
* time: 4 |  humidity: 67  | temperature: 25  | pressure: 101760 
* time: 5 |  humidity: 69  | temperature: 25  | pressure: 101761 
* time: 6 |  humidity: 70  | temperature: 25  | pressure: 101759 -> exhaling...
* time: 1 |  humidity: 71  | temperature: 25  | pressure: 101761 -> exhaling...
* time: 1 |  humidity: 71  | temperature: 25  | pressure: 101760 -> exhaling...
* time: 1 |  humidity: 70  | temperature: 25  | pressure: 101760 
* time: 2 |  humidity: 68  | temperature: 25  | pressure: 101761 
* time: 3 |  humidity: 67  | temperature: 25  | pressure: 101759 
* time: 4 |  humidity: 67  | temperature: 25  | pressure: 101760 
* time: 5 |  humidity: 70  | temperature: 25  | pressure: 101760 -> exhaling...
* time: 1 |  humidity: 71  | temperature: 25  | pressure: 101758 -> exhaling...
* time: 1 |  humidity: 72  | temperature: 25  | pressure: 101759 -> exhaling...
* time: 1 |  humidity: 73  | temperature: 25  | pressure: 101761 -> exhaling...
* time: 1 |  humidity: 71  | temperature: 25  | pressure: 101758 
* time: 2 |  humidity: 70  | temperature: 25  | pressure: 101760 
* time: 3 |  humidity: 68  | temperature: 25  | pressure: 101760 
* time: 4 |  humidity: 68  | temperature: 25  | pressure: 101763 
* time: 1 |  humidity: 71  | temperature: 25  | pressure: 101760 
* time: 2 |  humidity: 68  | temperature: 25  | pressure: 101761 
* time: 3 |  humidity: 66  | temperature: 25  | pressure: 101762 
* time: 4 |  humidity: 65  | temperature: 25  | pressure: 101759 
* time: 5 |  humidity: 65  | temperature: 25  | pressure: 101762 
* time: 6 |  humidity: 65  | temperature: 25  | pressure: 101762 
* time: 7 |  humidity: 64  | temperature: 25  | pressure: 101762 
* time: 8 |  humidity: 64  | temperature: 25  | pressure: 101764 
* time: 9 |  humidity: 63  | temperature: 25  | pressure: 101763 
* time: 10 |  humidity: 63  | temperature: 25  | pressure: 101765 
* time: 11 |  humidity: 62  | temperature: 25  | pressure: 101761 
* time: 12 |  humidity: 62  | temperature: 25  | pressure: 101760 
* time: 13 |  humidity: 62  | temperature: 25  | pressure: 101764 
* time: 14 |  humidity: 62  | temperature: 25  | pressure: 101763 
* time: 15 |  humidity: 62  | temperature: 25  | pressure: 101765 
* time: 16 |  humidity: 62  | temperature: 25  | pressure: 101766 
* time: 17 |  humidity: 61  | temperature: 25  | pressure: 101763 
* time: 18 |  humidity: 61  | temperature: 25  | pressure: 101765 
* time: 19 |  humidity: 60  | temperature: 25  | pressure: 101764 
* stop breathing for 10s! 
* end of a cycle
* restart...
* time: 0 |  humidity: 59  | temperature: 25  | pressure: 101763


### Algo2 – Decreasing and No-Changing Detection:
<strong> </strong> 

From Scenario 1 and Scenario 3, we found that when the person is not breathing. The humidity either remains the same as initial value or constantly decreases. That is, we can use this as a start condition of 10 seconds counting.

1.	When power on, the system runs <strong>5 seconds </strong> to sample the <strong>initial value (IV) </strong>  and store the value in a variable.
2.	The humidity fluctuates in the range between <strong> IV-2</strong> and <strong>IV+2 </strong> . When the fluctuation is in the range, we consider this as no    breathing, so the timer starts to count (or keeps counting).
3.	When the value falls out of the range, we stop and reset the timer.
4.	Another counting condition is <strong> when the value goes down </strong>(or fluctuates between -1 and +1), the timer starts to count.
5.	Until the timer reaches 10 sec, it triggers an alert.





