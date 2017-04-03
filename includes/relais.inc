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
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $url = variable_get('upei_roblib_ill_add_url', 'https://caul-cbua.relais-host.com/portal-service/request/add');
  $relais_arr = array(
    "SupplyingLibrarySymbol" => variable_get('upei_roblib_ill_library_symbol','PCU'),
  );
  $relais_arr['BibliographicInfo'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_request_form']);
  $relais_arr['DeliveryAddress'] = upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_auth_form']);
  $request_info = upei_roblib_ill_request_info_array($relais_arr['BibliographicInfo'], $form_state['values']['notes']);
  $relais_arr['RequestInfo'] = $request_info;
  $relais_json = json_encode($relais_arr);
  $options = array(
    'method' => 'POST',
    'data' => $relais_json,
    'timeout' => 15,
    'headers' => array('Content-Type' => 'application/json'),
  );
  $result = drupal_http_request($url . '?aid=' . $aid, $options);
  if ($result->code == '200') {
    $response_json = $result->data;
    $response_arr = json_decode($response_json);
    //TODO remove or improve the line below after testing
    drupal_set_message('submitted request to relais' . $response_json);
  }
  else {
    drupal_set_message(t('Error submitting request to relais, @code, @message', array(
      '@code' => $result->code,
      '@message' => $result->message
    )), 'error');
  }
}

/**
 * Request an aid from Relais.
 * @param string $barcode
 *   A patron_id, campus_id or barcode to identify a user
 * @return string/null
 *   Returns a Relais authentication id (token) or NULL on if authentication fails
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
  if ($result->code == '200') {
    $response_data = json_decode($result->data, TRUE);
    if (isset($response_data['Problem'])) {
      drupal_set_message(t('Error retrieving authentication token, @message', array('@message' => $response_data['Problem']['Message'])), 'error');
      return NULL;
    }
    $aid = $response_data['AuthorizationId'];
  }
  return isset($aid) ? $aid : NULL;

}

