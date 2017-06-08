<?php
/**
 * @file
 * the administrative form for upei_roblib_ill
 */


function upei_roblib_ill_admin_form($form, &$form_state) {
  $form = array();
  $form['upei_roblib_ill_add_url'] = array(
    '#required' => TRUE,
    '#type' => 'textfield',
    '#size' => 200,
    '#title' => t('The Relais AddRequest URL'),
    '#description' => t('The URL for the Relais ILL server AddRequest service, the default is 
    https://caul-cbua.relais-host.com/portal-service/request/add'),
    '#default_value' => variable_get('upei_roblib_ill_add_url', 'https://caul-cbua.relais-host.com/portal-service/request/add'),
  );
  $form['upei_roblib_ill_auth_url'] = array(
    '#required' => TRUE,
    '#type' => 'textfield',
    '#size' => 200,
    '#title' => t('The Relais Auth URL'),
    '#description' => t('The URL for the Relais ILL server Auth service (retrieve the aid), the default is 
    https://caul-cbua.relais-host.com/portal-service/user/authentication'),
    '#default_value' => variable_get('upei_roblib_ill_auth_url', 'https://caul-cbua.relais-host.com/portal-service/user/authentication'),
  );
  $form['upei_roblib_ill_library_symbol'] = array(
    '#required' => TRUE,
    '#type' => 'textfield',
    '#size' => 200,
    '#title' => t('Your Relais Library Symbol'),
    '#description' => t('Your Relais Library Symbol'),
    '#default_value' => variable_get('upei_roblib_ill_library_symbol'),
  );

  $form['upei_roblib_ill_relais_key'] = array(
    '#required' => TRUE,
    '#type' => 'textfield',
    '#size' => 200,
    '#title' => t('The Relais API key'),
    '#description' => t('The API key used to communicate with the Relais API, Contact Relais International to get your key'),
    '#default_value' => variable_get('upei_roblib_ill_relais_key'),
  );
  $form['upei_roblib_ill_doi_openurl_pid'] = array(
    '#required' => TRUE,
    '#type' => 'textfield',
    '#size' => 200,
    '#title' => t('OpenURL PID'),
    '#description' => t('An identifier to call yourself, for the OpenURL endpoint. To use this service you first need to register for an account here: http://www.crossref.org/requestaccount/'),
    '#default_value' => variable_get('upei_roblib_ill_doi_openurl_pid'),
  );
  $form['upei_roblib_ill_contact_email'] = array(
    '#required' => TRUE,
    '#type' => 'textfield',
    '#size' => 200,
    '#title' => t('Contact Email'),
    '#description' => t('The email address we want to show after a user has submitted an ILL request'),
    '#default_value' => variable_get('upei_roblib_ill_contact_email', 'ill@upei.ca'),
  );
  $form['upei_roblib_ill_contact_phone'] = array(
    '#required' => TRUE,
    '#type' => 'textfield',
    '#size' => 200,
    '#title' => t('Contact Phone Number'),
    '#description' => t('The phone number we want to show to the user after a user has submitted an ILL request'),
    '#default_value' => variable_get('upei_roblib_ill_contact_phone', '902-566-0445'),
  );


  return system_settings_form($form);

}
