<?php

function template_preprocess_hybridauth_widget(&$vars) {
  // We need to include the auth functions to get a list of enabled providers
  module_load_include('inc', 'hybridauth', 'hybridauth.auth');
  
  $vars['widget_id'] = drupal_html_id('hybridauth-widget');
  
  $vars['providers'] = array();
  foreach (hybridauth_get_enabled_providers() as $provider_id => $provider_name) {
    $icon_id = drupal_html_id('hybridauth-widget-provider-' . $provider_id);
    $icon_url = url(drupal_get_path('module', 'hybridauth_widget') . '/images/icons/' . strtolower($provider_id) . '.png');
    $vars['providers'][$provider_id] = l(
      theme('hybridauth_provider_icon', array('provider_id' => $provider_id)),
      'hybridauth/router',
      array(
        'html' => TRUE,
        'query' => array('provider' => $provider_id) + drupal_get_destination(),
        'attributes' => array('id' => $icon_id, 'class' => 'hybridauth-widget-provider'),
      )
    );
  }
  
  _hybridauth_add_widget_css();
}
