<?php
/**
 * @file
 * Contains the functions to send and receive data to/from Relais.
 */

/**
 * Submit an ILL AddRequest to relais.
 * @param array $form_state
 *   A drupal form_state array
 * @param string $aid
 *   A relais authentication id (token)
 */
function upei_roblib_ill_add_request($form_state, $aid) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/db');
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $url = variable_get('upei_roblib_ill_add_url', 'https://caul-cbua.relais-host.com/portal-service/request/add');
  $relais_arr = array(
    "SupplyingLibrarySymbol" => variable_get('upei_roblib_ill_library_symbol','PCU'),
  );
  $relais_arr['BibliographicInfo'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_request_form']);
  $relais_arr['DeliveryAddress'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_auth_form']);
  $request_info = upei_roblib_ill_request_info_array($relais_arr['BibliographicInfo'], $form_state['values']['notes']);
  $relais_arr['RequestInfo'] = $request_info;
  $relais_arr['PublisherInfo']['PublicationDate'] = isset($form_state['storage']['upei_roblib_ill_request_form']['Date']) ?
    $form_state['storage']['upei_roblib_ill_request_form']['Date'] : '';
  $relais_json = json_encode($relais_arr);
  $options = array(
    'method' => 'POST',
    'data' => $relais_json,
    'timeout' => 15,
    'headers' => array('Content-Type' => 'application/json'),
  );
  if(!isset($aid) || is_array($aid)) {
    upei_roblib_ill_log_request($relais_arr, $aid);
    drupal_set_message(t('Error retrieving authentication token, @message', array('@message' => $aid['Problem']['Message'])), 'error');
    return;
  }
  $result = drupal_http_request($url . '?aid=' . $aid, $options);
  module_load_include('inc', 'upei_roblib_ill', 'includes/db');
  $response_json = $result->data;
  $response_arr = json_decode($response_json, TRUE);
  if ($result->code == '200') {
    //TODO remove or improve the line below after testing
    drupal_set_message('submitted request to relais' . $response_json);
  }
  else {
    drupal_set_message(t('Error submitting request to relais, @code, @message', array(
      '@code' => $result->code,
      '@message' => $result->message
    )), 'error');
  }
  upei_roblib_ill_log_request($relais_arr, $response_arr);
}

/**
 * Request an aid from Relais.
 * @param string $barcode
 *   A patron_id, campus_id or barcode to identify a user
 * @return string/array
 *   Returns a Relais authentication id (token) or an array containing the Relais response error which should include the error message if authentication fails
 */
function upei_roblib_ill_authenticate($barcode) {
  $url = variable_get('upei_roblib_ill_auth_url', 'https://caul-cbua.relais-host.com/portal-service/user/authentication');
  $json_arr = array();
  $json_arr['ApiKey'] = variable_get('upei_roblib_ill_relais_key');
  $json_arr['UserGroup'] = 'patron';
  $json_arr['LibrarySymbol'] = variable_get('upei_roblib_ill_library_symbol');
  $json_arr['PatronId'] = $barcode;
  $request_json = json_encode($json_arr);
  $options = array(
    'method' => 'POST',
    'data' => $request_json,
    'timeout' => 15,
    'headers' => array('Content-Type' => 'application/json'),
  );
  $result = drupal_http_request($url, $options);
  $response_data = json_decode($result->data, TRUE);
  if ($result->code == '200') {
    if (isset($response_data['Problem'])) {
      return $response_data;
    }
    $aid = $response_data['AuthorizationId'];
  }
  return isset($aid) ? $aid : $response_data;

}

