<?php

function template_preprocess_hybridauth_widget(&$vars, $hook) {
  // We need to include the auth functions to get a list of enabled providers
  module_load_include('inc', 'hybridauth', 'hybridauth.auth');
  
  $style = $vars['style'];
  $size = $vars['size'];
  
  $vars['widget_id'] = drupal_html_id($hook);
  $vars['classes_array'][] = drupal_html_class('hybridauth-style-' . $style);
  $vars['classes_array'][] = drupal_html_class('hybridauth-size-' . $size);
  
  if ($style == 'providers') {
    $providers = hybridauth_get_enabled_providers();
  }
  else {
    $providers = array(
      'list' => t('Sign in using a social networking account'),
    );
  }
  
  $vars['providers'] = array();
  foreach ($providers as $provider_id => $provider_name) {
    $icon_id = drupal_html_id('hybridauth-widget-provider-' . $provider_id);
    $icon_url = url(drupal_get_path('module', 'hybridauth_widget') . '/images/icons/' . strtolower($provider_id) . '.png');
    $icon_class = array(
      'hybridauth-widget-provider',
    );
    
    $query = array('provider' => $provider_id, 'style' => $vars['style']) + drupal_get_destination();
    
    if (variable_get('hybridauth_widget_use_overlay', HYBRIDAUTH_WIDGET_USE_OVERLAY_DEFAULT)) {
      if (module_exists('colorbox')) {
        // We need to load the colorbox-load behavior
        drupal_add_js(drupal_get_path('module', 'colorbox') . '/js/colorbox_load.js');
        
        $query['overlay'] = 'true';
        
        //$icon_class[] = 'colorbox';
        $icon_class[] = 'colorbox-load';
        
        $query['width'] = 430;
        $query['height'] = 222;
        $query['iframe'] = 'true';
      }
    }
    
    $vars['providers'][$provider_id] = l(
      theme('hybridauth_provider_icon', array('size' => $size, 'provider_id' => $provider_id)),
      'hybridauth/popup',
      array(
        'html' => TRUE,
        'query' => $query,
        'attributes' => array('id' => $icon_id, 'class' => $icon_class),
      )
    );
  }
  
  _hybridauth_add_widget_css();
}

function template_preprocess_hybridauth_popup_loading(&$vars, $hook) {
  $provider_id = $vars['provider_id'];
  $done = $vars['done'];
  $overlay = $vars['overlay'];
  
  $vars['throbber_url'] = url(drupal_get_path('module', 'hybridauth') . '/images/throbber.gif');
  $vars['destination'] = isset($_GET['destination']) ? $_GET['destination'] : 'user';
  
  if ($done && $overlay) {
    drupal_add_js('
      if (window.opener) {
        try { window.opener.parent.$.colorbox.close(); } catch(err) {}
        window.opener.parent.location.href = "' . url($vars['destination']) . '";
      }
      window.self.close();
    ', 'inline');
  }
  elseif ($done) {
    drupal_add_js('
      window.location.href = "' . url($vars['destination']) . '";
    ', 'inline');
    watchdog('hybridauth', print_r($vars, TRUE));
  }
  else {
    drupal_add_js('
      window.location.href = window.location.href + "&authenticate=1";
    ', 'inline');
  }
}

function template_preprocess_hybridauth_popup_provider(&$vars, $hook) {
  $provider_id = $vars['provider_id'];
  
  $destination = isset($_GET['destination']) ? $_GET['destination'] : 'user';
  unset($_GET['destination']);
  
  $query = drupal_get_query_parameters();
  $query['destination'] = $destination;
  
  $vars['provider_name'] = hybridauth_get_provider_name($provider_id);
  $vars['provider_icon'] = theme('hybridauth_provider_icon', array('size' => 64, 'provider_id' => $provider_id));
  
  $overlay = $vars['overlay'];
  
  $button_id = drupal_html_id('hybridauth-popup-' . $provider_id . '-button');
  $vars['provider_button'] = l(
    t('Login'),
    'hybridauth/popup',
    array(
      'query' => $query,
      'attributes' => array('id' => $button_id, 'class' => array()),
    )
  );
  
  // TODO: handle cancel button
  $vars['cancel_button'] = 'Use another account';
  
  $url = url('hybridauth/popup', array('query' => $query + array('authenticate' => TRUE)));
  drupal_add_js('
    (function ($) {
      Drupal.behaviors.hybridauthPopup = {
        attach: function (context, settings) {
          $("#' . $button_id . '", context).once("' . $button_id . '", function() {
            $(this).click(function(event) {
              event.preventDefault();
              window.open(
                "' . $url . '",
                "hybridauth_social_sing_on",
                "location=0,status=0,scrollbars=0,width=800,height=500"
              );
              return false;
            });
          });
        }
      };
    }(jQuery));
  ', 'inline');
}
