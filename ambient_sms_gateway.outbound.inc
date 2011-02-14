<?php

class ambientOutboundSms {

  public $transaction;
  public $id;
  public $destination;
  public $message;
  public $sent_at;
  public $options;
  public $api_key;
  public $api_password;
  public $api_sms_url;
  public $send_status;
  public $send_status_message;
  public $response_message;
  public $response_message_id;

  private $_status_map = array();

  function _init_status_map() {
   $this->_status_map = array (
    0 => t('Message successfully submitted.'),
    1 => t('Message successfully submitted, but contained recipient errors'),
    2 => t('Message successfully submitted, but contained duplicates and recipient errors'),
    1000 => t('Message successfully submitted, but contained duplicates.'),
    1001 => t('Invalid HTTP POST.'),
    1002 => t('HTTP Post Empty no XML string found.'),
    1003 => t('Malformed XML.'),
    1004 => t('Invalid XML.'),
    1005 => t('Authentication Error: API-Key empty.'),
    1006 => t('Authentication Error: API-Key invalid.'),
    1007 => t('Authentication Error: Password empty.'),
    1008 => t('Authentication fail.'),
    1009 => t('No recipients found.'),
    1010 => t('Invalid Recipient(s)'),
    1011 => t('Message body empty.'),
    1012 => t('Message body exceeds maximum message length.'),
    1013 => t('Message body contains invalid characters.'),
    1014 => t('There were duplicates found.'),
    1015 => t('Authentication Error: IP address not allowed.'),
    1016 => t('Invalid message id.'),
    1017 => t('Insufficient credits.'),
    1018 => t('Account suspended.'),
    1019 => t('Account deactivated.'),
   );
 }

  function __construct($destination, $message, $conf) {
    $this->destination = $destination;
    $this->message = $message;

    //Status Map
     $this->_init_status_map();

    //Configuration
    $this->options = $conf['options'];
    $this->api_key = $conf['ambient_api_key'];
    $this->api_password = $conf['ambient_api_password'];
    $this->api_sms_url = $conf['ambient_api_sms_url'];

    //Transaction log
    $this->transaction = new stdClass();
  }

  function statusMessage($num) {
    if(!$this->_status_map[$num]) {
      return t('Reserved for future use');
    }

    return $this->_status_map[$num];
  }

  function send($log = TRUE) {

    $xml = $this->getXML();
    $this->transaction->request = $xml;

    if($log) $this->logTransaction();

    $xml = $this->getXML(); // We call this again to make sure the transaction ID is included to support replies. Its a trivial call and shouldnt matter.
    $this->transaction->request = $xml;

    $http_result = drupal_http_request($this->api_sms_url, array("Content-Type"=>"text/xml","Expect"=>NULL), 'POST', $xml);
    $this->sent_at = time();

    if($http_result->code == 200) {
      try {
        $this->transaction->response = $http_result->data;
        $return_xml = new SimpleXMLElement($http_result->data);
        if(is_object($return_xml)) {
          
          if($return_xml->status == "0" || $return_xml->status == "1" || $return_xml->status == "2") {
            $this->send_status = TRUE;
          } else {
            $this->send_status = FALSE;
          }

          $status = intval($return_xml->status);
          $this->response_code = $status;
          $this->send_status_message = $this->statusMessage($status);

          $this->response_message = (string) $return_xml->message;
          $this->response_message_id = (string) $return_xml->message_id;
        } else {
          $this->send_status = FALSE;
          $this->send_status_message = t('Could not parse the XML response');
          $this->response_message = $this->send_status_message;
          $this->response_message_id = 0;
        }
      } catch (Exception $e) {
          $this->send_status = FALSE;
          $this->send_status_message = t('Could not parse the XML response');
          $this->response_message = $this->send_status_message;
          $this->response_message_id = 0;
          $this->transaction->response = "<xml>" . $this->send_status_message . "</xml>";
      }
    } else {
      $this->send_status = FALSE;
      $this->send_status_message = t("%code error while communicating with %gateway", array("%code" => $http_result->code, "%gateway" => $this->api_sms_url));
      $this->response_message = $this->send_status_message;
      $this->response_message_id = 0;
      $this->transaction->response = "<xml>" . $this->send_status_message . "</xml>";
    }

    if($log) $this->logTransaction();
  
    return $this->send_status;
  }

  function getXML() {

    $re_url = url("gateway/ambient/inbound.php", array('absolute' => TRUE));

    $xml = "<sms><api-key>{$this->api_key}</api-key><password>{$this->api_password }</password><recipients><mobile>{$this->destination}</mobile></recipients><msg>".htmlspecialchars($this->message)."</msg><reply_path>{$re_url}</reply_path><message_id>{$this->id}</message_id></sms>";

    return $xml;
  }

  function logTransaction() {
    if($this->id) {
      drupal_write_record("ambient_sms_gateway_outbox", $this, "id");
      drupal_write_record("ambient_sms_gateway_outbox_transaction", $this->transaction, "id");
    } else {
      drupal_write_record("ambient_sms_gateway_outbox", $this);
      
      //Set  the transaction id if it isnt yet set
      if($this->id && !$this->transaction->id) { $this->transaction->id = $this->id; }
      drupal_write_record("ambient_sms_gateway_outbox_transaction", $this->transaction);
    }
  }
}
