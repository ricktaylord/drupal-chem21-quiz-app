<?php
class CHEM21Course {
  public function __construct($node=NULL,$nid=NULL) {
    if($node) {
      $this->_node = $node;
      $this->nid = $node->nid;
    } elseif($nid) {
      $this->nid=$nid;
    } else {
      // bad arguments
      throw new Exception("Either a node or nid must be passed to the CHEM21Course constructor");
    }
  }
  public function node() {
    if(!isset($this->_node)) {
      $this->_node = node_load($this->nid);
    }
    return $this->_node;
  }

  public function _delete_quiz_weights() {
    db_delete('opigno_quiz_app_quiz_sort')
    ->condition('gid', $this->nid)
    ->execute();
  }

  public function reorder_lessons($quiz_weights) {
    $this->_delete_quiz_weights();
    $query = db_insert('opigno_quiz_app_quiz_sort')
    ->fields(array('gid','quiz_nid','weight'));


    foreach($quiz_weights as $nid=>$weight) {
      try {
        $record = array('gid'=>intval($this->nid), 'quiz_nid'=>intval($nid), 'weight'=>intval($weight));
        $query->values($record);
      } catch (Exception $e ) {
        return services_error("Bad add values ".$e->getMessage, 500);
      }
    }
    try {
      $query->execute();
    } catch (Exception $e) {
      return services_error("Bad execute ".$e->getMessage, 500);
    }
    return $quiz_weights;
  }
}