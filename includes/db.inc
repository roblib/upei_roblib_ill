<?php
/**
 * @file
 * functions for database access.
 */


function upei_roblib_ill_log_request($request, $respose) {
  try {
    db_insert('upei_roblib_ill_request')
      ->fields(array(
        'patron_id' => $request['DeliveryAddress']['campus_id'],
        'patron_firstname' => $request['DeliveryAddress']['FirstName'],
        'patron_lastname' => $request['DeliveryAddress']['Surname'],
        'patron_type' => $request['DeliveryAddress']['patron_type'],
        'patron_department' => $request['DeliveryAddress']['Department'],
        'patron_email' => $request['DeliveryAddress']['DeliveryAddress'],
        'notes' => $request['DeliveryAddress']['notes'],
        'genre' => $request['BibliographicInfo']['Genre'],
        'author' => $request['BibliographicInfo']['Author'],
        'citation_date' => $request['BibliographicInfo']['Date'],
        'title' => $request['DeliveryAddress']['Title'],
        'atitle' => $request['DeliveryAddress']['ArticleTitle'],
        'issn' => $request['DeliveryAddress']['issn'],
        'isbn' => $request['DeliveryAddress']['isbn'],
        'article_author' => $request['DeliveryAddress']['ArtigleAuthor'],
        'volume' => $request['DeliveryAddress']['Volume'],
        'issue' => $request['DeliveryAddress']['Issue'],
        'pages_requested' => $request['DeliveryAddress']['PagesRequested'],
        'time_submitted' => strtotime($request['RequestInfo']['DateSubmitted']),
      ))->execute();
  } catch(Exception $e) {
    watchdog_exception('upei_roblib_ill',  $e, 'Error logging ILL request.', array(), WATCHDOG_ERROR);
  }
}
