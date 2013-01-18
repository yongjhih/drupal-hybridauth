<?php
/**
 * @file
 * Functions for integrating the Rules module with HybridAuth.
 */

/**
 * Implements hook_rules_condition_info().
 * @ingroup rules
 */
function hybridauth_rules_condition_info() {
  $conditions = array();
  
  $conditions['hybridauth_user_created_hybridauth'] = array(
    'label' => t('User created by HybridAuth'),
    'module' => t('HybridAuth'),
    'arguments' => array(
      'account' => array('label' => t('User'), 'type' => 'user'),
    ),
    'help' => t('TRUE is returned if user account was created by HybridAuth.'),
  );
  
  return $conditions;
}

function hybridauth_user_created_hybridauth($account) {
  global $hybridauth_data;
  return (!empty($hybridauth_data) || !empty($account->hybridauth));
}
