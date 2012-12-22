<?php
/**
 * 
 */

function hybridauth_ulogin_migrate_form($form, &$form_state) {
  $form = array();
  
  $form['message'] = array(
    '#markup' => format_plural(_hybridauth_ulogin_identities_count(), 'There is 1 uLogin identity to process.', 'There are @count uLogin identities to process.'),
  );
  
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Process'),
  );
  return $form;
}

function hybridauth_ulogin_migrate_form_submit($form, &$form_state) {
  $batch = array(
    //'title' => t('Processing'),
    //'init_message' => t('Initializing.'),
    //'progress_message' => t('Completed @current of @total.'),
    //'error_message' => t('An error has occurred.'),
    'operations' => array(
      array('_hybridauth_ulogin_batch_operation', array()),
    ),
    'finished' => '_hybridauth_ulogin_finished_callback',
    'file' => drupal_get_path('module', 'hybridauth_ulogin') . '/hybridauth_ulogin.admin.inc',
  );
  batch_set($batch);
}

function _hybridauth_ulogin_batch_operation(&$context) {
  if (empty($context['sandbox'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = _hybridauth_ulogin_identities_count();
  }

  $limit = 2;
  $identities = _hybridauth_ulogin_identities($context['sandbox']['progress'], $limit);

  foreach ($identities as $id => $identity) {
    _hybridauth_ulogin_identity_process($identity);
    
    $context['results'][] = $id . ' ' . $identity->network . ' ' . _hybridauth_ulogin_provider_convert($identity->network);
    $context['sandbox']['progress']++;
  }
  $context['message'] = t('Processed uLogin identity !id', array('!id' => $id));
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

function _hybridauth_ulogin_finished_callback($success, $results, $operations) {
  if ($success) {
    drupal_set_message(format_plural(count($results), 'Processed 1 uLogin identity.', 'Processed @count uLogin identities.'));
  }
  else {
    drupal_set_message(t('An error has occurred.'), 'error');
  }
}

function _hybridauth_ulogin_identities_count() {
  $result = db_select('ulogin_identity', 'ul_id')
    ->countQuery()
    ->execute()
    ->fetchField();
  return $result;
}

function _hybridauth_ulogin_identities($start = 0, $limit = 100) {
  $result = db_select('ulogin_identity', 'ul_id')
    ->fields('ul_id')
    ->range($start, $limit)
    ->execute()
    ->fetchAllAssoc('id');
  return $result;
}

function _hybridauth_ulogin_identity_process($identity) {
  if ($provider = _hybridauth_ulogin_provider_convert($identity->network)) {
    $data = array(
      'provider' => $provider,
      'identifier' => $identity->ulogin_uid,
    );
    if (!_hybridauth_identity_load($data)) {
      _hybridauth_identity_save($data, $identity->uid);
    }
  }
}

function _hybridauth_ulogin_providers_map() {
  $providers_map = &drupal_static(__FUNCTION__, NULL);
  if (!isset($providers_map)) {
    //ulogin network key => hybridauth provider key
    $providers_map = array(
      'vkontakte' => 'Vkontakte',
      'odnoklassniki' => 'Odnoklassniki',
      'mailru' => 'Mailru',
      'facebook' => 'Facebook',
      'twitter' => 'Twitter',
      'google' => 'Google',
      'yandex' => 'Yandex',
      'livejournal' => '',
      'openid' => 'OpenID',
      'lastfm' => 'LastFM',
      'linkedin' => 'LinkedIn',
      'liveid' => 'Live',
      'soundcloud' => '',
      'steam' => '',
      'flickr' => '',
      'vimeo' => '',
      'youtube' => '',
      'webmoney' => '',
    );
  }
  return $providers_map;
}

function _hybridauth_ulogin_provider_convert($ulogin_network_key) {
  $providers_map = _hybridauth_ulogin_providers_map();
  
  if (array_key_exists($ulogin_network_key, $providers_map)) {
    return $providers_map[$ulogin_network_key];
  }
  
  return FALSE;
}
