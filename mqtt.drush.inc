<?php
/**
 * Implements hook_drush_command().
 */
function mqtt_drush_command() {

  $items['mqtt-listen'] = array(
    'description' => 'Start the MQTT processor.',
    'aliases' => array('mqtt-l'),
  );

  $items['mqtt-publish'] = array(
    'description' => 'Publish an MQTT event.',
    'aliases' => array('mqtt-p'),
  );

  return $items;
}

/**
 * Fire a test event.
 */
function drush_mqtt_publish() {
  include_once(drupal_get_path('module', 'mqtt') . '/phpMQTT/phpMQTT.php');

  $mqtt = new phpMQTT(variable_get('mqtt_host', 'test.mosquitto.org'), variable_get('mqtt_port', '1883'), variable_get('mqtt_client_name', 'Drupal MQTT client name') . 'publish');
  $mqtt->debug=TRUE;

  if(!$mqtt->connect()){
    watchdog('MQTT', 'Failed to create MQTT connection');
    echo 'Connection failed';
    exit;
  }

  // Hardcode the test message.
  $mqtt->publish('/yourtopic/xxx', 'Testing message', 0);
  $mqtt->close();
}


/**
 * Callback for the drush-demo-command command.
 */
function drush_mqtt_listen() {

  include_once(drupal_get_path('module', 'mqtt') . '/phpMQTT/phpMQTT.php');

  $mqtt = new phpMQTT(variable_get('mqtt_host', 'test.mosquitto.org'), variable_get('mqtt_port', '1883'), variable_get('mqtt_client_name', 'Drupal MQTT client name'));
$mqtt->debug=TRUE;

  if(!$mqtt->connect()){
    watchdog('MQTT', 'Failed to create MQTT connection');
    exit;
  }

  $topics = array();
  // Work out which topics we want to subscribe to.
  $rules = rules_config_load_multiple(FALSE);
  foreach($rules as $rule) {
    foreach($rule->events() as $event) {
      if ($event == 'mqtt_subscribe') {
        // We are interested in this rule.
        foreach($rule->conditions() as $condition) {
          if ($condition->settings['data:select'] == 'mqtt-topic') {
            $topics[$condition->settings['value']] = array(
              "qos" => 0,
              "function" => "mqtt_handle_event"
            );
          }
        }
      }
    }
  }

  if (count($topics)) {
    $mqtt->subscribe($topics,0);

    while($mqtt->proc()){

    }

    $mqtt->close();
  }
  else {
    print 'No topics to subscribe to';
    exit;
  }
}

/**
 * Fire the rule.
 * @todo  - something clever here to handle wildcards.
 */
function mqtt_handle_event($topic, $msg) {
  // Based on the topic - fire the relevant events.
  if (module_exists('rules')) {
    print 'DEBUG - firing event for ' . $topic . "\n";
    rules_invoke_event('mqtt_subscribe', $topic);
  }
}
