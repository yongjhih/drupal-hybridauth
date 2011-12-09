<?php

function hybridauth_get_providers() {
  $providers = &drupal_static(__FUNCTION__, NULL);
  
  if (!isset($providers)) {
    $raw_providers = array(
      'OpenID'      => 'OpenID',
      'Facebook'    => 'Facebook',
      'Twitter'     => 'Twitter',
      'LinkedIn'    => 'LinkedIn',
      'MySpace'     => 'MySpace',
      'Google'      => 'Google',
      'Yahoo'       => 'Yahoo',
      'YahooOAuth'  => 'Yahoo (OAuth)',
      'Foursquare'  => 'Foursquare',
      'AOL'         => 'AOL',
      'Live'        => 'Windows Live',
    );
    $providers = array();
    $weights = array();
    
    foreach ($raw_providers as $provider_id => $provider_name) {
      $weights[$provider_id] = variable_get('hybridauth_provider_' . $provider_id . '_weight', 0);
    }
    asort($weights);
    
    foreach ($weights as $provider_id => $weight) {
      $providers[$provider_id] = $raw_providers[$provider_id];
    }
  }
  
  return $providers;
}

function hybridauth_get_enabled_providers() {
  $providers = &drupal_static(__FUNCTION__, NULL);
  
  if (!isset($providers)) {
    $providers = array();
    
    foreach (hybridauth_get_providers() as $provider_id => $provider_name) {
      if ($provider_config = hybridauth_get_provider_config($provider_id)) {
        $providers[$provider_id] = $provider_name;
      }
    }
  }
  
  return $providers;
}

function hybridauth_get_provider_name($provider_id) {
  $providers = hybridauth_get_providers();
  return isset($providers[$provider_id]) ? $providers[$provider_id] : NULL;
}

function hybridauth_get_controller() {
  $controller = &drupal_static(__FUNCTION__, NULL);
  
  if (!isset($controller)) {
    $controller = FALSE;
    
    $lib_path = libraries_get_path('hybridauth', FALSE);
    if ($lib_path) {
      try {
        require_once($lib_path . '/Hybrid/Auth.php');
        
        $config = hybridauth_get_config();
        $controller = new Hybrid_Auth($config);
      }
      catch(Exception $e) {
        // TODO: How should we handle this?
        watchdog_exception('hybridauth', $e);
      }
    }
  }
  
  return $controller;
}

function hybridauth_get_config() {
  $config = &drupal_static(__FUNCTION__, NULL);
  
  if (!isset($config)) {
    //$lib_path = libraries_get_path('hybridauth', FALSE);
    $config = array(
      'base_url' => url('hybridauth/endpoint', array('absolute' => TRUE)),
      'providers' => array(),
    );
    
    foreach (hybridauth_get_providers() as $provider_id => $provider_name) {
      if ($provider_config = hybridauth_get_provider_config($provider_id)) {
        $config['providers'][$provider_id] = $provider_config;
      }
    }
  }
  
  return $config;
}

function hybridauth_get_provider_config($provider_id, $enabled_only = TRUE) {
  $enabled = variable_get('hybridauth_provider_' . $provider_id . '_enabled', NULL);
  if (isset($enabled)) {
    if (!$enabled_only || $enabled) {
      return array(
        'enabled' => $enabled,
        'keys' => array(
          'id' => variable_get('hybridauth_provider_' . $provider_id . '_keys_id', ''),
          'key' => variable_get('hybridauth_provider_' . $provider_id . '_keys_key', ''),
          'secret' => variable_get('hybridauth_provider_' . $provider_id . '_keys_secret', ''),
        ),
        'scope' => variable_get('hybridauth_provider_' . $provider_id . '_scope', ''),
      );
    }
  }
  
  return NULL;
}

