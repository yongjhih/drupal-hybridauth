<?php

function template_preprocess_hybridauth_widget(&$vars, $hook) {
  $element = $vars['element'];
  $vars['widget_id'] = drupal_html_id($element['#hybridauth_id']);
  
  $vars['providers'] = array();
  
  if ($element['#hybridauth_widget_type'] == 'list') {
    foreach (hybridauth_get_enabled_providers() as $provider_id => $provider_name) {
      $vars['providers'][$provider_id] = l(
        theme('hybridauth_provider_icon', array('icon_pack' => $element['#hybridauth_icon_pack'], 'provider_id' => $provider_id, 'provider_name' => $provider_name)),
        'hybridauth/popup/' . $provider_id,
        array(
          'html' => TRUE,
          'query' => drupal_get_destination(),
          'attributes' => array(
            'id' => drupal_html_id('hybridauth-widget-provider-' . $provider_id),
            'title' => $provider_name,
            'class' => array('hybridauth-widget-provider')),
        )
      );
    }
  }
  else {
    $provider_id = 'none';
    if (variable_get('hybridauth_widget_use_overlay', 1)) {
      $class = array('ctools-use-modal', 'ctools-modal-hybridauthmodal');
      ctools_include('modal');
      ctools_modal_add_js();
      $settings = array(
        'hybridauthmodal' => array(
          'modalSize' => array(
            'type' => 'scale',
            'width' => '400px',
            'height' => '200px',
            'addWidth' => 0,
            'addHeight' => 0,
            'contentRight' => 25,
            'contentBottom' => 45,
          ),
          'modalTheme' => 'HybridAuthModalDialog',
          //'throbberTheme' => 'HybridAuthModalThrobber',
          'modalOptions' => array(
            'opacity' => 0.55,
            'background' => '#000',
          ),
          'animation' => 'fadeIn',
          'animationSpeed' => 'slow',
          'closeText' => t('Close'),
          'closeImage' => '',
          //'loadingText' => t('Loading...'),
          //'throbber' => theme('image', array(
          //  'path' => ctools_image_path('throbber.gif'),
          //  'title' => t('Loading...'),
          //  'alt' => t('Loading'),
          //)),
        ),
      );
      drupal_add_js($settings, 'setting');
    }
    
    $vars['providers'][$provider_id] = l(
      ($element['#hybridauth_widget_type'] == 'link') ?
        variable_get('hybridauth_widget_link_text', 'Social network account') :
        theme('hybridauth_provider_icon', array('icon_pack' => $element['#hybridauth_icon_pack'], 'provider_id' => $provider_id, 'provider_name' => variable_get('hybridauth_widget_link_title', 'Social network account'))),
      'hybridauth/providers/nojs',
      array(
        'html' => TRUE,
        'query' => drupal_get_destination(),
        'attributes' => array(
          'id' => drupal_html_id('hybridauth-widget-provider-' . $provider_id),
          'title' => variable_get('hybridauth_widget_link_title', 'Social network account'),
          'class' => $class,
        ),
      )
    );
  }
  
  _hybridauth_add_icon_pack_files($element['#hybridauth_icon_pack']);
}
