<?php
/**
 * @file
 * Contains helper functions for working with Relais requests.
 */

/**
 * Removes empty elements and drupal specific elements from an array
 *
 * @param $values
 *   A drupal form_state['values] array
 * @return array
 *   An array where the keys in the array that have empty values are removed and
 * drupal specific elements are removed.
 */
function upei_roblib_ill_clean_array($values) {
  $arr = array_filter($values);
  if (isset($arr['ISSN'])) {
    $arr['ISSN'] = array($arr['ISSN']);
  }
  if (isset($arr['ISBN'])) {
    $arr['ISBN'] = array($arr['ISBN']);
  }
  if(isset($arr['Genre']) && $arr['Genre'] == 'article' && isset($arr['Author'])) {
    // We want ArticleAuthor
    unset($arr['Author']);
  }
  unset($arr['form_build_id']);
  unset($arr['form_token']);
  unset($arr['form_id']);
  unset($arr['op']);
  unset($arr['submit']);
  unset($arr['next']);
  unset($arr['certify']);
  unset($arr['doi']);
  unset($arr['doi_button']);
  return $arr;
}

/**
 * Allows us to ask for _REQUEST variables that may or may not exist without generating a php warning.
 * Also does some formatting for certain variables.
 * @param string $variable
 *   The name of the $_REQUEST variable to check
 * @return string
 *   The value of the $_REQUEST variable or an empty string.
 */
function upei_roblib_ill_get_request_variable($variable) {
  if ($variable == 'author' && empty($_REQUEST[$variable])) {
    //google scholar usually sends auinit aulast instead of author
    $initial = isset($_REQUEST['auinit']) ? $_REQUEST['auinit'] : NULL;
    $last_name = isset($_REQUEST['aulast']) ? $_REQUEST['aulast'] : NULL;
    return !empty($last_name) ? $last_name . ',' . $initial : '';
  }
  return isset($_REQUEST[$variable]) ? $_REQUEST[$variable] : '';
}

/**
 * Populates the requestInfo portion of a Relais request
 * @param array $values
 *   an array containing a 'Genre' element.
 * @return array
 *   The requestInfo portion of a Relais request
 */
function upei_roblib_ill_request_info_array($values, $notes) {
  $requestInfo = array();
  $genre = $values['Genre'];
  $requestInfo['DateSubmitted'] = date('Y-m-d H:i:s');
  $requestInfo['Notes'] = $notes;
  if ($genre == 'book' || $genre == 'chapter') {
    $requestInfo['ServiceType'] = 'L';
    $requestInfo['ServiceLabel'] = "R";
    //$requestInfo['RequestSource'] = "C";
  }
  else {
    $requestInfo['ServiceType'] = 'X';
    $requestInfo['ServiceLabel'] = "R";
    //$requestInfo['RequestSource'] = "C";
  }
  return $requestInfo;
}

/**
 * Creates a summary Table based on the biblio information in the form_state array for display
 * @param $form_state
 * @return string
 */
function upei_roblib_format_biblio_info($form_state) {
  $data =  upei_roblib_ill_clean_array($form_state['storage']['upei_roblib_ill_request_form']);
  $rows = array();
  foreach($data as $key => $value) {
    if($key == 'ISSN' || $key == 'ISBN') {
      $value = reset($value);
    }
    array_push($rows, array($key, $value));
  }
  $output = array(
    '#theme' => 'table',
    '#rows' => $rows,
    '#prefix' => '<div class=upei-roblib-ill-biblio-info>',
    '#suffix' => '</div>',
  );
  $html_output = drupal_render($output);
  return $html_output;
}
