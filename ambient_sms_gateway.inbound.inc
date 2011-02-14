<?php

class ambientInboundSms {

  public $transaction;
  public $id;
  public $sender;
  public $destination;
  public $message;
  public $in_reply_to;
  public $received_at;
  public $options;

  function __construct($sender, $destination, $message, $received_at, $in_reply_to, $conf) {
    $this->sender = $sender;
    $this->destination = $destination;
    $this->message = $message;
    $this->received_at = $received_at;
    $this->in_reply_to = $in_reply_to;

    //Configuration
    $this->options = $conf['options'];

    //Transaction log
    $this->transaction = new stdClass();
  }

  static function parseInbound() {

    $POST = file_get_contents("php://input");

    // msisdn=27826941134&shortcode=2782007210000006&keyword=Hi&msg=Hi+dude&message_id=1
    $ret = new ambientInboundSms($_POST['msisdn'], $_POST['shortcode'], $_POST['msg'], time(), $_POST['message_id'], array()); 
    $ret->transaction->request = $POST;
    return $ret;

  }

  function logTransaction() {
    if($this->id) {
      drupal_write_record("ambient_sms_gateway_inbox", $this, "id");
      drupal_write_record("ambient_sms_gateway_inbox_transaction", $this->transaction, "id");
    } else {
      drupal_write_record("ambient_sms_gateway_inbox", $this);
      
      //Set  the transaction id if it isnt yet set
      if($this->id && !$this->transaction->id) { $this->transaction->id = $this->id; }
      drupal_write_record("ambient_sms_gateway_inbox_transaction", $this->transaction);
    }
  }
}
