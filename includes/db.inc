<?php
/**
 * @file
 * functions for database access.
 */


/**
 * Log an ILL request
 *
 * @param array $request
 *   The request sent to Relais as an array (before it is encoded as JSON)
 * @param array $response
 *   The response from Relais.
 */
function upei_roblib_ill_log_request($request, $response) {
  $time_submitted = isset($request['RequestInfo']['DateSubmitted']) ? $request['RequestInfo']['DateSubmitted'] : '';
  $relais_message = isset($response['ConfirmMessage']) ? $response['ConfirmMessage'] : $response['Problem']['Message'];
  try {
    db_insert('upei_roblib_ill_request')
      ->fields([
        'patron_id' => $request['DeliveryAddress']['campus_id'],
        'patron_firstname' => $request['DeliveryAddress']['FirstName'],
        'patron_lastname' => $request['DeliveryAddress']['Surname'],
        'patron_type' => $request['DeliveryAddress']['patron_type'],
        'patron_department' => $request['DeliveryAddress']['Department'],
        'patron_email' => $request['DeliveryAddress']['DeliveryAddress'],
        'notes' => isset($request['DeliveryAddress']['notes']) ? $request['DeliveryAddress']['notes'] : '',
        'genre' => isset($request['BibliographicInfo']['Genre']) ? $request['BibliographicInfo']['Genre'] : '',
        'doi' => isset($request['BibliographicInfo']['AdditionalNumbers']) ? $request['BibliographicInfo']['AdditionalNumbers'] : '',
        'author' => isset($request['BibliographicInfo']['Author']) ? $request['BibliographicInfo']['Author'] : '',
        'citation_date' => isset($request['BibliographicInfo']['Date']) ? $request['BibliographicInfo']['Date'] : '',
        'title' => isset($request['BibliographicInfo']['Title']) ? $request['BibliographicInfo']['Title'] : '',
        'atitle' => isset($request['BibliographicInfo']['ArticleTitle']) ? $request['BibliographicInfo']['ArticleTitle'] : '',
        'issn' => isset($request['BibliographicInfo']['ISSN'][0]) ? $request['BibliographicInfo']['ISSN'][0] : '',
        'isbn' => isset($request['BibliographicInfo']['ISBN'][0]) ? $request['BibliographicInfo']['ISBN'][0] : '',
        'article_author' => isset($request['BibliographicInfo']['ArticleAuthor']) ? $request['BibliographicInfo']['ArticleAuthor'] : '',
        'volume' => isset($request['BibliographicInfo']['Volume']) ? $request['BibliographicInfo']['Volume'] : '',
        'issue' => isset($request['BibliographicInfo']['Issue']) ? $request['BibliographicInfo']['Issue'] : '',
        'pages_requested' => isset($request['BibliographicInfo']['PagesRequested']) ? $request['BibliographicInfo']['PagesRequested'] : '',
        'time_submitted' => strtotime($time_submitted),
        'relais_request_id' => isset($response['RequestNumber']) ? $response['RequestNumber'] : '-1',
        'relais_message' => substr($relais_message, 0, 254),
      ])->execute();
  } catch (Exception $e) {
    watchdog_exception('upei_roblib_ill', $e, 'Error logging ILL request.', NULL, WATCHDOG_ERROR);
  }
}
