<?php
/**
 * @file
 * Contains the ILL forms.
 */

/**
 * A form to wrap the different steps of our multi step form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the bibliographic form.
 */
function upei_roblib_ill_form($form, &$form_state) {
  if (!isset($form_state['values'])) {
    $form_name = 'upei_roblib_ill_request_form';
  }
  else {
    $form_name = 'upei_roblib_ill_auth_form';
  }
  $form_state['step'] = $form_name;
  return $form_name === 'upei_roblib_ill_request_form' ? upei_roblib_ill_request_form($form, $form_state) :
    upei_roblib_ill_auth_form($form, $form_state);
}


/**
 * Validation for the upei_roblib_ill_form form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function upei_roblib_ill_form_validate($form, &$form_state) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/relais');
  if ($form_state['step'] == 'upei_roblib_ill_auth_form') {
    if (!valid_email_address($form_state['values']['DeliveryAddress'])) {
      form_set_error('DeliveryAddress', t('Your email address appears to be invalid.'));
    }
    if (!$form_state['values']['certify']) {
      form_set_error('certify', t('Please certify that this is item is being sought for a fair dealing purpose'));
    }
    $form_state['values']['campus_id'] = upei_roblib_ill_form_message_id($form_state['values']['campus_id']);
    $aid = upei_roblib_ill_authenticate($form_state['values']['campus_id'], $form_state['values']['Surname']);
    if (is_array($aid) && isset($aid['Problem']['Message'])) {
      form_set_error('Surname', $aid['Problem']['Message']);
      form_set_error('campus_id', '');
      // Log invalid requests, we need to build the full array here to log it, even though we won't send it yet as auth has failed.
      $form_state['storage']['upei_roblib_ill_auth_form'] = $form_state['values'];
      module_load_include('inc', 'upei_roblib_ill', 'includes/db');
      $arr = upei_roblib_ill_build_relais_arr($form_state);
      upei_roblib_ill_log_request($arr, $aid);
    }
    else {
      $form_state['storage']['aid'] = $aid;
    }
  }
  if ($form_state['step'] == 'upei_roblib_ill_request_form' && $form_state['triggering_element']['#value'] !== 'Lookup DOI' && empty($form_state['values']['Title'])) {
    form_set_error('Title', t('Journal/Book Title is required.'));
  }

}

/**
 * @param string $id
 *   The users campus id
 *
 * @return mixed
 *   the campus id with leading zeros and leading/trailing whitespace removed.
 */
function upei_roblib_ill_form_message_id($id) {
  $id = ltrim($id, '0');
  return trim($id);
}

/**
 * Submit handler for the upei_roblib_ill_form form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function upei_roblib_ill_form_submit($form, &$form_state) {

  $form_state['storage'][$form_state['step']] = $form_state['values'];
  switch ($form_state['step']) {
    case 'upei_roblib_ill_request_form':
      if ($form_state['clicked_button']['#value'] != 'Lookup DOI') {
        $form_state['step'] = 'upei_roblib_ill_auth_form';
        $form_state['rebuild'] = TRUE;
      }
      break;
    case 'upei_roblib_ill_auth_form':
      //process the form
      $response = upei_roblib_ill_add_request($form_state, $form_state['storage']['aid']);
      $error = isset($response['RequestNumber']) ? 'FALSE' : 'TRUE';

      $form_state['redirect'] = [
        '/upei/roblib/ill/finished',
        [
          'query' => [
            'message' => $response['ConfirmMessage'],
            'email' => $form_state['values']['DeliveryAddress'],
            'error' => $error,
          ],
        ],
      ];
  }

}

/**
 * After submitting an ILL request patrons will be redirected to the content
 * returned from this function.
 *
 * @return string
 *   a string of HTML to display including the results of the ILL request and
 *   some boilerplate text.
 */
function upei_roblib_ill_form_redirect() {
  $contact_email = variable_get('upei_roblib_ill_contact_email', 'ill@upei.ca');
  $contact_phone_number = variable_get('upei_roblib_ill_contact_phone', '902-566-0445');
  $librarian_link = l(t('subject-specific'), 'http://library.upei.ca/librarians', ['html' => TRUE]);
  $help_message = t("We can help! Please contact your !librarian_link librarian if you'd like help finding more resources relating to your topic.",
    ['!librarian_link' => $librarian_link]);
  $standard_message = t("To contact the department about this request, you can send a message to @email or
call @phone.", ['@phone' => $contact_phone_number, '@email' => $contact_email]);
  if (isset($_GET['error']) && $_GET['error'] === 'FALSE') {
    $standard_message = t("A message including the Request ID has been sent to @email.", [
        '@email' => $_GET['email'],
      ]) . $standard_message;
  }

  $output = [
    '#prefix' => '<div upei-roblib-ill-content>',
    '#suffix' => '</div>',
    'dynamic_message' => [
      '#type' => 'markup',
      '#markup' => $_GET['message'],
      '#prefix' => '<div class="upei-roblib-ill-message">',
      '#suffix' => '</div>',
    ],
    'standard_message' => [
      '#type' => 'markup',
      '#markup' => $standard_message,
      '#prefix' => '<div class="upei-roblib-ill-email">',
      '#suffix' => '</div>',
    ],
    'help_message' => [
      '#type' => 'markup',
      '#markup' => $help_message,
      '#allowed_tags' => ['a', 'source'],
      '#prefix' => '<div class="upei-roblib-ill-help">',
      '#suffix' => '</div>',
    ],
  ];
  return drupal_render($output);
}


/**
 * The patron portion of the ILL form.
 *
 * The campus_id and email portion are also used in a request for an Relais aid.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the bibliographic form.
 */
function upei_roblib_ill_auth_form($form, &$form_state) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $form['#prefix'] = upei_roblib_format_biblio_info($form_state);
  $form['#suffix'] = '<div class="roblib-required">* = Required Field</div>
<div class="roblib-FOIPP"><strong>Protection of Privacy</strong> - The personal information requested 
on this form is collected under the authority of Section 31(c) of the PEI 
Freedom of Information and Protection of Privacy Act and will be protected
 under Part 2 of that Act. It will be used for the purpose of administering 
 and providing library service. Direct any questions about this collection to
  Health Sciences and Scholarly Communications Librarian at ill@upei.ca, 550
   University Ave, Charlottetown PE , 902-566-0453.</div>';
  $form['FirstName'] = [
    '#type' => 'textfield',
    '#title' => t('Your First Name'),
    '#size' => 50,
    '#required' => TRUE,
  ];
  $form['Surname'] = [
    '#type' => 'textfield',
    '#title' => t('Your Last Name'),
    '#size' => 50,
    '#required' => TRUE,
  ];
  $form['campus_id'] = [
    '#type' => 'textfield',
    '#title' => t('Your Campus ID'),
    '#size' => 50,
    '#required' => TRUE,
  ];
  $patron_types = [
    'Faculty/Staff' => t('Faculty/Staff'),
    'Undergraduate Student' => t('Undergraduate Student'),
    'Masters Student' => t('Masters Student'),
    'Doctoral Student' => t('Doctoral Student'),
    'Alumni Premiere Subscriber' => t('Alumni Premiere Subscriber'),
    'Alumni/Public' => t('Alumni/Public'),
    'Other' => 'Other',
  ];
  $departments = [
    'Actuarial Science' => t('Actuarial Science'),
    'Analytics' => t('Analytics'),
    'Applied Communication, Leadership, and Culture' => t('Applied Communication, Leadership, and Culture'),
    'Applied Health Services Research' => t('Applied Health Services Research'),
    'Applied Human Sciences' => t('Applied Human Sciences'),
    'Arts (General)' => t('Arts (General)'),
    'Biology' => t('Biology'),
    'Business' => t('Business'),
    'Canadian Studies' => t('Canadian Studies'),
    'Chemistry' => t('Chemistry'),
    'Climate Change and Adaptation' => t('Climate Change and Adaptation'),
    'Computer Science' => t('Computer Science'),
    'Diversity and Social Justice Studies' => t('Diversity and Social Justice Studies'),
    'Economics' => t('Economics'),
    'Education' => t('Education'),
    'English' => t('English'),
    'Environmental Studies/Sciences' => t('Environmental Studies/Sciences'),
    'Financial Mathematics' => t('Financial Mathematics'),
    'Foods and Nutrition' => t('Foods and Nutrition'),
    'French' => t('French'),
    'Global Affairs' => t('Global Affairs'),
    'History' => t('History'),
    'Integrated Studies' => t('Integrated Studies'),
    'Island Studies' => t('Island Studies'),
    'Journalism' => t('Journalism'),
    'Kinesiology' => t('Kinesiology'),
    'Library, Robertson' => t('Library, Robertson'),
    'Nursing' => t('Nursing'),
    'Mathematics' => t('Mathematics'),
    'Modern Languages' => t('Modern Languages'),
    'Music' => t('Music'),
    'Paramedicine' => t('Paramedicine'),
    'Philosophy' => t('Philosophy'),
    'Physics' => t('Physics'),
    'Political Science' => t('Political Science'),
    'Psychology' => t('Psychology'),
    'Radiography' => t('Radiography'),
    'Religious Studies' => t('Religious Studies'),
    'Science (General)' => t('Science (General)'),
    'Sociology/Anthropology' => t('Sociology/Anthropology'),
    'Spanish' => t('Spanish'),
    'Statistics' => t('Statistics'),
    'Sustainable Design Engineering' => t('Sustainable Design Engineering'),
    'Wildlife Conservation' => t('Wildlife Conservation'),
    'Veterinary Medicine' => t('Veterinary Medicine'),
    'Other' => ('Other (please add to the notes field)'),
  ];
  $form['patron_type'] = [
    '#type' => 'select',
    '#title' => t('Your Patron Type'),
    '#options' => $patron_types,
    '#required' => TRUE,
  ];
  $form['Department'] = [
    '#type' => 'select',
    '#title' => t('Your Department'),
    '#options' => $departments,
    '#required' => TRUE,
  ];
  $form['DeliveryAddress'] = [
    '#type' => 'textfield',
    '#title' => t('Your Email (If using an email address other than @upei.ca, enter that address in the Notes field as well.)'),
    '#size' => 50,
    '#required' => TRUE,
  ];
  $form['InstitutionName'] = [
    '#type' => 'hidden',
    '#title' => t('Name of Institution'),
    '#size' => 50,
    '#default_value' => 'University of Prince Edward Island',
    '#required' => TRUE,
  ];
  $form['LibraryName'] = [
    '#type' => 'hidden',
    '#title' => t('Name of Library'),
    '#size' => 50,
    '#default_value' => 'Robertson Library',
    '#required' => TRUE,
  ];
  $form['notes'] = [
    '#type' => 'textarea',
    '#title' => t('Notes'),
    '#size' => 50,
    '#required' => FALSE,
  ];
  $form['certify'] = [
    '#title' => t('I certify'),
    '#type' => 'checkbox',
    '#description' => 'I hereby certify that this item is being sought for a fair dealing purpose and any copying will be in accordance with the Copyright Act. ',
    '#size' => 50,
    '#required' => TRUE,
  ];
  $form['submit'] = [
    '#type' => 'submit',
    '#value' => 'Submit',
  ];

  return $form;

}


/**
 * The bibliographic portion of the ILL form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the bibliographic form.
 */
function upei_roblib_ill_request_form($form, &$form_state) {
  module_load_include('inc', 'upei_roblib_ill', 'includes/utilities');
  $form['#prefix'] = '<div id="upei-roblib-ill-request-form">';
  $form['#suffix'] = '<div class="roblib-required">* = Required Field</div></div>';
  $type = upei_roblib_ill_get_request_variable('genre');
  $genre = empty($type) ? 'article' : $type;
  $form['Genre'] = [
    '#type' => 'select',
    '#title' => t('Item Type'),
    '#options' => [
      'book' => t('Book'),
      'chapter' => t('Book Chapter'),
      'article' => t('Journal Article'),
    ],
    '#required' => TRUE,
  ];
  if(!empty($type)) {
    $form['Genre']['#default_value'] = $type;
  }
  $form['doi'] = [
    '#type' => 'textfield',
    '#title' => t("DOI (ex: 10.1016/j.ypmed.2019.01.018) <a href='https://library.upei.ca/doi'>more info</a>"),
    '#default_value' => upei_roblib_ill_get_doi_from_request(),
    //'#description' => 'DOI, if you provide a DOI we will attempt to lookup the record for you',
    '#size' => 50,
    '#states' => [
      'invisible' => [
        ':input[name="Genre"]' => ['value' => 'book'],
      ],
    ],
    '#required' => FALSE,
  ];
  $form['doi_button'] = [
    '#type' => 'submit',
    '#value' => 'Lookup DOI',
    '#ajax' => [
      'callback' => 'upei_roblib_ill_doi_callback',
      'method' => 'replace',
      'wrapper' => 'upei-roblib-ill-request-form',
      'effect' => 'fade',
    ],
    '#states' => [
      'invisible' => [
        ':input[name="Genre"]' => ['value' => 'book'],
      ],
    ],
    '#required' => FALSE,
  ];
  $form['Title'] = [
    '#type' => 'textarea',
    '#rows' => 2,
    '#title' => t('Journal/Book Title *'),
    '#default_value' => upei_roblib_ill_get_request_variable('title'),
    //'#description' => 'Journal or Book Title (Required)',
    '#size' => 50,
    '#required' => FALSE,
  ];
  $form['ArticleTitle'] = [
    '#type' => 'textarea',
    '#rows' => 2,
    '#title' => t('Article/Chapter Title'),
    '#default_value' => upei_roblib_ill_get_request_variable('atitle'),
    //'#description' => 'Article or Chapter Title',
    '#size' => 50,
    '#states' => [
      'invisible' => [
        ':input[name="Genre"]' => ['value' => 'book'],
      ],
    ],
    '#required' => FALSE,
  ];

  $form['ISSN'] = [
    '#type' => 'textfield',
    '#title' => t('ISSN (ex: 1234-567X)'),
    '#default_value' => upei_roblib_ill_get_request_variable('issn'),
    '#size' => 50,
    '#states' => [
      'visible' => [
        ':input[name="Genre"]' => ['value' => 'article'],
      ],
    ],
    '#required' => FALSE,
  ];
  $article_author = ($genre == 'book') ? '' : upei_roblib_ill_get_request_variable('author');
  $form['ArticleAuthor'] = [
    '#type' => 'textfield',
    '#title' => t('Article/Chapter Author'),
    '#default_value' => $article_author,
    '#size' => 50,
    '#required' => FALSE,
    //'#description' => 'Article or Chapter Author',
    '#states' => [
      'invisible' => [
        ':input[name="Genre"]' => ['value' => 'book'],
      ],
    ],
  ];
  $author = ($genre == 'book') ? upei_roblib_ill_get_request_variable('author') : '';
  $form['Author'] = [
    '#type' => 'textfield',
    '#title' => t('Book Author'),
    '#default_value' => $author,
    '#size' => 50,
    '#required' => FALSE,
    '#states' => [
      'invisible' => [
        ':input[name="Genre"]' => ['value' => t('article')],
      ],
    ],
  ];
  $form['ISBN'] = [
    '#type' => 'textfield',
    '#title' => t('ISBN (ex: 9780323554459)'),
    '#default_value' => upei_roblib_ill_get_request_variable('isbn'),
    '#size' => 50,
    '#states' => [
      'invisible' => [
        ':input[name="Genre"]' => ['value' => 'article'],
      ],
    ],
    '#required' => FALSE,
  ];
  $form['SubTitle'] = [
    '#type' => 'hidden',
    '#title' => t('Sub Title'),
    '#default_value' => upei_roblib_ill_get_request_variable('ubtitle'),
    '#size' => 50,
    '#required' => FALSE,
  ];
  $form['SeriesTitle'] = [
    '#type' => 'hidden',
    '#title' => t('Series Title'),
    '#default_value' => upei_roblib_ill_get_request_variable('stitle'),
    '#size' => 50,
    '#required' => FALSE,
  ];

  $form['Volume'] = [
    '#type' => 'textfield',
    '#title' => t('Volume'),
    '#default_value' => upei_roblib_ill_get_request_variable('volume'),
    '#size' => 50,
    '#required' => FALSE,
    '#states' => [
      'visible' => [
        ':input[name="Genre"]' => ['value' => 'article'],
      ],
    ],
  ];
  $form['Issue'] = [
    '#type' => 'textfield',
    '#title' => t('Issue'),
    '#default_value' => upei_roblib_ill_get_request_variable('issue'),
    '#size' => 50,
    '#required' => FALSE,
    '#states' => [
      'visible' => [
        ':input[name="Genre"]' => ['value' => 'article'],
      ],
    ],
  ];
  $form['PagesRequested'] = [
    '#type' => 'textfield',
    '#title' => t('Pages (startpage-endpage)'),
    '#default_value' => upei_roblib_ill_get_request_variable('pages'),
    //'#description' => 'Pages requested (startpage-endpage)',
    '#size' => 50,
    '#required' => FALSE,
    '#states' => [
      'invisible' => [
        ':input[name="Genre"]' => ['value' => 'book'],
      ],
    ],
  ];
  $form['Date'] = [
    '#type' => 'textfield',
    '#title' => t('Publication Date'),
    '#default_value' => upei_roblib_ill_get_request_variable('date'),
    '#size' => 50,
    '#required' => FALSE,
  ];
  $form['next'] = [
    '#type' => 'submit',
    '#value' => t('Next'),
  ];
  return $form;

}
