#ifndef LINENTOOLS_TELEMETRY_METRIC_REPORTER_H
#define LINENTOOLS_TELEMETRY_METRIC_REPORTER_H

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>

enum MetricReporterRunType {
    METRIC_ON_CHANGE,
    METRIC_ON_INTERVAL,
    METRIC_ON_INTERVAL_WITH_SMOOTHING,
    METRIC_ON_PAUSE,
    METRIC_ON_RESUME,
};

enum MetricReporterRunFlag {
    RUN_FLAG_UPDATE_TOPIC_MQTT_BASE,
    RUN_FLAG_UPDATE_RETAINED_EVENT,
    RUN_FLAG_UPDATE_QOS_EVENT,
    RUN_FLAG_UPDATE_CHANGE_THRESHOLD,
    RUN_FLAG_UPDATE_POLLING_INTERVAL,
    RUN_FLAG_UPDATE_DELAY_INTERVAL,
    RUN_FLAG_UPDATE_COMPLETE,
    RUN_FLAG_UPDATE_RUN_TYPE_PAUSED,
    RUN_FLAG_UPDATE_RUN_TYPE_RESUMED,
    RUN_FLAG_UPDATE_RUN_TYPE_ON_INTERVAL,
    RUN_FLAG_UPDATE_RUN_TYPE_ON_INTERVAL_SMOOTHING,
    RUN_FLAG_UPDATE_RUN_TYPE_ON_CHANGE,
};

class TelemetryMetricReporter {
    protected:
      /* connection */
      MqttClient* _mqttClient;

      /* run vars */
      MetricReporterRunType _runType;
      MetricReporterRunType _runTypePrevious;
      MetricReporterRunFlag _runFlag;

      unsigned long _msDelayInterval;
      unsigned long _msDelaySample;
      unsigned long _msDelayPolling;
      float        _thresholdChange;

      unsigned long _msLastOperation;

      /* value flags */
      float _metricValuePrevious;

      /* action flags */
      bool _isConfigChanged = false;

      /* base MQTT topic for messages */
      String _topicMqttBase;

      /* metric target name for responding to metric action requests */
      String _targetName;

      /* MQTT message settings for metric events */
      bool _retainEvents = true;
      uint8_t _qosEvents = 0;

      /* sampling */
      unsigned long _msLastSample = 0;
      uint8_t _sampleCount = 0;
      uint8_t _sampleSize;
      float _sampleAverage = 0;
      float _sampleTotal = 0;

    /* protected methods */
        virtual String _getTopicEvents() = 0;

        /* PUBLISH METHODS */
        void _publishEvent(String eventName) {
            String stringEventTopic = _getTopicEvents();
            int topicLength = stringEventTopic.length() + 1;
            char charEventTopic[topicLength];

            stringEventTopic.toCharArray(charEventTopic, topicLength);

            if(_mqttClient == nullptr) {
              Serial.println("!!! --- TelemetryMetricReporter->_mqttClient is nullptr! Did you remember to call 'begin'? --- !!!");
            }

            _mqttClient->beginMessage(
            charEventTopic,
            _retainEvents,
            _qosEvents
            );

            _mqttClient->print(eventName);
            _mqttClient->endMessage();
        }

        void _publishJson(
            JsonDocument json,
            String topic,
            bool retain,
            uint8_t qos
        ) {
            _mqttClient->beginMessage(
                topic,
                retain,
                qos
            );

            serializeJson(json, *_mqttClient);

            _mqttClient->endMessage();
        }

        JsonDocument _getBaseConfig() {
            JsonDocument json; 

            if (_runType == METRIC_ON_CHANGE) {
                json["run"]["on"] =  "CHANGE";
                json["run"]["msPoll"] = _msDelayPolling; 
                json["run"]["thresh"] = _thresholdChange;
            }

            if (_runType == METRIC_ON_INTERVAL) {
                json["run"]["on"] =  "INTERVAL";
                json["run"]["msInterval"] = _msDelayInterval;
            }

            json["events"]["msgBase"] = _topicMqttBase;
            json["events"]["msgRetain"] = _retainEvents;
            json["events"]["msgQos"] = _qosEvents;

            return json;
        }

    /* returns true if the caller should immediately return */
    bool _runEventHandler() {
        if (_isConfigChanged) {
            publishConfiguration();
            _isConfigChanged = false; // reset change flag
            _runFlag = RUN_FLAG_UPDATE_COMPLETE; // flag the state as copmlete
            return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_TOPIC_MQTT_BASE) {
            yield();
            _publishEvent("UPDATE_EVENT_TOPIC_MQTT_BASE");
            _isConfigChanged = true;
            return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_RETAINED_EVENT) {
            yield();
            _publishEvent("UPDATE_EVENT_PUB_RETAIN");
            _isConfigChanged = true;
            return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_QOS_EVENT) {
            yield();
            _publishEvent("UPDATE_EVENT_PUB_QOS");
            _isConfigChanged = true;
            return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_CHANGE_THRESHOLD) {
            yield();
            _publishEvent("UPDATE_EVENT_CHANGE_THRESHOLD");
            _isConfigChanged = true;
            return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_POLLING_INTERVAL) {
            yield();
            _publishEvent("UPDATE_EVENT_POLLING_INTERVAL");
            _isConfigChanged = true;
            return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_DELAY_INTERVAL) {
            yield();
            _publishEvent("UPDATE_EVENT_INTERVAL_DELAY");
            _isConfigChanged = true;
            return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_RUN_TYPE_PAUSED) {
          yield();
          _publishEvent("UPDATE_EVENT_RUN_TYPE_PAUSED");
          _isConfigChanged = true;
          return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_RUN_TYPE_RESUMED) {
          yield();
          _publishEvent("UPDATE_EVENT_RUN_TYPE_RESUMED");
          _isConfigChanged = true;
          return true;          
        }

        if (_runFlag == RUN_FLAG_UPDATE_RUN_TYPE_ON_INTERVAL) {
          yield();
          _publishEvent("UPDATE_EVENT_RUN_TYPE_CHANGED_TO_ON_INTERVAL");
          _isConfigChanged = true;
          return true;          
        }

        if (_runFlag == RUN_FLAG_UPDATE_RUN_TYPE_ON_INTERVAL_SMOOTHING) {
          yield();
          _publishEvent("UPDATE_EVENT_RUN_TYPE_CHANGED_T0_INTERVAL_SMOOTHING");
          _isConfigChanged = true;
          return true;
        }

        if (_runFlag == RUN_FLAG_UPDATE_RUN_TYPE_ON_CHANGE) {
          yield();
          _publishEvent("UPDATE_EVENT_RUN_TYPE_CHANGED_TO_ON_CHANGE");
          _isConfigChanged = true;
          return true;          
        }

        return false;
    }

    public:
      TelemetryMetricReporter(
        String targetName,
        String topicMqttBase,
        bool retainEventMessages,
        uint8_t qosEventMessages
      ): _targetName(targetName), _topicMqttBase(topicMqttBase), _retainEvents(retainEventMessages), _qosEvents(qosEventMessages) {};

      /* abstract methods */
      virtual void publishMetric() = 0;
      virtual void publishConfiguration() = 0;
      virtual float getMetricValue() = 0;

      /* inherited function */
      virtual void begin(
        MqttClient* mqttClient
       ) {
        /* set vars */
        _mqttClient = mqttClient;
    
        /* flag configuration as changed so it publishes */
        _isConfigChanged = true;
      }

    
      /* run operations */
      void setRunTypeInterval(unsigned long msDelayInterval) {
        _runType = METRIC_ON_INTERVAL;
        _runFlag = RUN_FLAG_UPDATE_RUN_TYPE_ON_INTERVAL;
        _msDelayInterval = msDelayInterval;
        _msLastOperation = _msDelayInterval + millis(); // timeout the interval
      }

      void setRunTypeOnChange(unsigned long msDelayPolling, float thresholdChange) {
        _runType = METRIC_ON_CHANGE;
        _runFlag = RUN_FLAG_UPDATE_RUN_TYPE_ON_CHANGE;
        _msDelayPolling = msDelayPolling;
        _thresholdChange = thresholdChange;
      }

      void setRunTypeIntervalWithSmoothing(
        unsigned long msDelayInterval,
        unsigned long msDelaySample, 
        uint8_t sampleSize) {
        _runType = METRIC_ON_INTERVAL_WITH_SMOOTHING;
        _runFlag = RUN_FLAG_UPDATE_RUN_TYPE_ON_INTERVAL_SMOOTHING;
        _msDelayInterval = msDelayInterval;
        _msDelaySample = msDelaySample;
        _sampleSize = sampleSize;
        _sampleCount = 0;
        _sampleAverage = 0;
        _sampleTotal = 0;
        _msLastOperation = millis() + msDelayInterval;
      } 

      void pauseReporting() {
        _runTypePrevious = _runType; // record the previous state
        _runType = METRIC_ON_PAUSE; // set the run type to paused
        _runFlag = RUN_FLAG_UPDATE_RUN_TYPE_PAUSED;
      }

      void resumeReporting() {
        _runType = METRIC_ON_RESUME; // set the run type to paused
        _runFlag = RUN_FLAG_UPDATE_RUN_TYPE_RESUMED;
      }

      void run() {
        /* handle events */
        if (_runEventHandler()) {
            return;
        }

        if (_runType == METRIC_ON_PAUSE) {
          return; // paused, do nothing
        }

        if (_runType == METRIC_ON_RESUME) {
          _runType = _runTypePrevious;
        }

        if (_runType == METRIC_ON_INTERVAL_WITH_SMOOTHING) {
          /* check if interval is up */
          if (millis() - _msLastOperation < _msDelayInterval) {
            return; // too early
          }

          /* interval is timed out, collect samples */
          if (_sampleCount >= _sampleSize) {
            /* we've collected enough samples and we're ready to publish! */
            publishMetric();
            /* reset sample values */
            _sampleCount = 0;
            _sampleTotal = 0;
            _sampleAverage = 0;
            /* reset our interval timeout */
            _msLastOperation = millis();
            return;
          }

          /* we still need samples, keep sampling */
          /* check if polling interval has passed */
          if (millis() - _msLastSample < _msDelayPolling) {
            return; // not enough time has passed, don't sample again
          }

          _sampleTotal += getMetricValue(); // add current sensor reading
          _sampleCount++; // increment sample count
          _sampleAverage = _sampleTotal/_sampleCount; //re-calc average
          
          /* reset the polling interval */
          _msDelayPolling = millis();
        }

        if (_runType == METRIC_ON_INTERVAL) {
            /* check if the interval has timed out */
            if (millis() - _msLastOperation < _msDelayInterval) {
                return; // interval hasn't timed out yet, do nothing
            }

            /* interval has timed, publish */
            publishMetric(); // publish the metric
            _msLastOperation = millis(); // update the timestamp
        }

        if (_runType == METRIC_ON_CHANGE) {
            /* check if the polling delay has timed out */
            if (millis() - _msLastOperation < _msDelayPolling) {
                return; // not ready to poll again, do nothing
            }

            /* read the metric value*/
            float metricValueCurrent = getMetricValue();

            /* check if current value matches previous value */
            if (metricValueCurrent == _metricValuePrevious) {
                return; // no change, do nothing
            }

            /* check if the change exceeds the threshold for reporting */
            if (abs(metricValueCurrent - _metricValuePrevious) < _thresholdChange) {
                return; // change less than threshold, do nothing
            }

            /* change exceeds threshold */
            publishMetric(); // publish metric 
            _msLastOperation = millis(); // update timestamp
            _metricValuePrevious = metricValueCurrent; // update the previous flag
        }

        return;
      }

      void onMessage(JsonDocument json) {
        /* read values from json */
        String targetName = json["target"];

        if (targetName != _targetName) {
          return;
        }

        int action = json["action"];
        unsigned long msDelay = json["msDelay"];
        unsigned long msPoll = json["msPoll"];
        uint8_t sampleSize = json["sampleSize"];
        double threshold = json["threshold"];

        if (action == 200) { // rquest to pause reporting
          pauseReporting();
          return;
        }

        if (action  == 201) { // resume reporting
          resumeReporting();
          return; 
        }

        if (action == 202) { // request report onChange
          setRunTypeOnChange(msDelay, threshold);
          return;
        }

        if (action == 203) { // request report on interval
          setRunTypeInterval(msDelay);
          return;
        }

        if (action == 204) {
          setRunTypeIntervalWithSmoothing(
            msDelay,
            sampleSize,
            msPoll
          );
        }
      }

    /* EVENTING CONFIGURATION */
      void setTopicMqttBase(String topic) {
        _topicMqttBase =  topic;
        _runFlag = RUN_FLAG_UPDATE_TOPIC_MQTT_BASE;
      }

      void setRetainEvents(bool isRetained) {
        _retainEvents = isRetained; /* update the variable */
        _runFlag = RUN_FLAG_UPDATE_RETAINED_EVENT;
      }

      void setQosEvents(uint8_t qos) {
        _qosEvents = qos;
        _runFlag = RUN_FLAG_UPDATE_QOS_EVENT;
      }

      /* ON CHANGE CONFIGURATION */
      void setChangeThreshold(float thresholdChange) {
        _thresholdChange = thresholdChange;
        _runFlag = RUN_FLAG_UPDATE_CHANGE_THRESHOLD;
      }

      void setPollingInterval(unsigned long msPollingInterval) {
        _msDelayPolling = msPollingInterval;
        _runFlag = RUN_FLAG_UPDATE_POLLING_INTERVAL;
      }

      void setDelayInterval(unsigned long msDelayInterval) {
        _msDelayInterval = msDelayInterval;
        _runFlag = RUN_FLAG_UPDATE_DELAY_INTERVAL;
      }

      /**
       * Returns the sample average when running with smoothing. 
       * If smoothing isn't being run, this will return 0
       */
      float getSmoothedMetricValue() {
        return _sampleAverage;
      }      
};

#endif