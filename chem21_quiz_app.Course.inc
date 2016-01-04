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

  public function add_tools($tools) {
    $fields = array('entity_type','bundle','entity_id','revision_id','language','delta','opigno_course_tools_tool');
    $query1 = db_insert('field_data_opigno_course_tools')->fields($fields);
    $query2 = db_insert('field_revision_opigno_course_tools')->fields($fields);
    db_delete('field_data_opigno_course_tools')
    ->condition('entity_id', $this->nid)
    ->condition('revision_id', $this->node()->vid)
    ->execute();
    db_delete('field_revision_opigno_course_tools')
    ->condition('entity_id', $this->nid)
    ->condition('revision_id', $this->node()->vid)
    ->execute();


    foreach($tools as $i=>$tool) {
      $record = array('entity_type'=>'node','bundle'=>'course','language'=>'und','entity_id'=>$this->nid,'revision_id'=>$this->node()->vid,'delta'=>$i,'opigno_course_tools_tool'=>$tool);
      #return services_error(print_r($record));
      try {
        $query1->values($record);
      } catch (Exception $e) {
        return services_error("Bad add data tools values ".$e->getMessage, 500);
      }
      try {
        $query2->values($record);
      } catch (Exception $e) {
        return services_error("Bad add revision tools values ".$e->getMessage, 500);
      }
    }
    try {
      $query1->execute();
    } catch (Exception $e) {
      return services_error("Bad add data tools execute ".print_r($record,TRUE), 500);
    }
    try {
      $query2->execute();
    } catch (Exception $e) {
      return services_error("Bad add revision tool execute ".$e->getMessage, 500);
    }
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