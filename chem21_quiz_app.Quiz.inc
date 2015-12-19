<?php
class CHEM21Quiz {
  public function __construct($node=NULL,$nid=NULL,$vid=NULL) {
    if($node) {
      $this->_node = $node;
      $this->nid = $node->nid;
      $this->vid = $node->vid;
    } elseif($nid && $vid) {
      $this->nid=$nid;
      $this->vid=$vid;
    } else {
      // bad arguments
      throw new Exception("Either a node or nid/vid pair must be passed to the CHEM21Quiz constructor");
    }
  }
  public function node() {
    if(!isset($this->_node)) {
      $this->_node = node_load($this->nid,$this->vid);
    }
    return $this->_node;
  }
  public function num_remaining_questions() {
    $n= count($this->get_remaining_questions());

    return $n;
  }
  public function question_list() {
    if(!isset($this->_question_list)) {
      $this->_question_list = quiz_build_question_list($this->node());
      $flist = quiz_get_questions($this->nid, $this->vid, TRUE, TRUE, TRUE, TRUE);
      foreach($this->_question_list as &$question) {
        $number = $question['number'];
        $question = $flist[$question['nid']];
        $question->number = $number;
      }
    }
    return $this->_question_list;
  }
  public function question_list_by_nid() {
    if(!isset($this->_question_list_by_nid)) {
      $qlist = $this->question_list();
      $olist=array();
      foreach($qlist as $question) {
        $olist[$question->nid] = $question;
      }
      $this->_question_list_by_nid = $olist;
    }
    return $this->_question_list_by_nid;
  }
  public function is_live() {
    return isset($_SESSION['quiz_' . $this->nid]['quiz_questions']);
  }
  public function is_current() {
    $curr_obj = menu_get_object("node");
    return $curr_obj->nid == $this->nid;
  }
  private function get_remaining_questions() {
    return $_SESSION['quiz_' . $this->nid]['quiz_questions'];
  }
  public function get_current_question_number() {
    $number_of_questions = $this->get_number_of_questions();
    return $number_of_questions - $this->num_remaining_questions() + 1;
  }
  public function get_number_of_questions() {
    if(!isset($this->_num_of_questions)) {
      $this->_num_of_questions = quiz_get_number_of_questions($this->vid);
    }
    return $this->_num_of_questions;
  }
  public function get_question_number_from_nid($nid) {
    $qlist = $this->question_list_by_nid();
    return $qlist[$nid]->number;
  }
  public function get_current_question() {
    $qlist = $this->question_list();
    $question = $qlist[$this->get_current_question_number()-1];
    return $question;
  }
  public function rid() {
    if(!isset($this->_rid)) {
      Global $user;
      if (!$this->is_live()) {
        // We delete questions in progress from old revisions.
        _quiz_delete_old_in_progress($this->node(), $user->uid);
        // See if the current user has progress for this revision of the quiz stored
        // in the database
        $rid = $user->uid > 0 ? _quiz_active_result_id($user->uid, $this->nid, $this->vid) : 0;
        // Are we resuming an in-progress quiz
        if ($this->node()->allow_resume && $rid > 0) {
          _quiz_resume_existing_quiz($this->node(), $user->uid, $rid);
          $this->_rid=$rid;
          return $rid;
        }
        // First time running through quiz.
        elseif (quiz_start_check($this->node(), $rid)) {
          _quiz_take_quiz_init($this->node());
        }
      }
      $this->_rid = _quiz_active_result_id($user->uid, $this->nid, $this->vid);
    }
    return $this->_rid;
  }
  public static function is_question_node($node) {
    $types = _quiz_get_question_types();

    return array_key_exists($node->type,$types);
  }
  public static function get_quizzes_for_question_vid($vid) {
    $sql = 'SELECT n.nid, r.parent_vid AS vid, r.weight, n.title FROM {quiz_node_relationship} r
                  JOIN {node} n ON n.nid = r.parent_nid
                  WHERE r.child_vid = :child_vid
                  ORDER BY r.parent_vid ASC';
    $res = db_query($sql, array(':child_vid' => $vid));
    $out=array();
    while ($res_o = $res->fetch()) {
      $out[] = $res_o->nid;
    }
    return $out;
  }
  public function jump_to_question($question_node) {
    $question_num = $this->get_question_number_from_nid($question_node->nid);
    if($this->rid()) {
      quiz_jump_to($question_num,$this->node(),$this->rid());
      return TRUE;
    } 
    return FALSE;
  }
  public function render_current_question() {
      $question = $this->get_current_question();
      $to_be_rendered = quiz_take_quiz($this->node());
      $new_question = $this->get_current_question();

      if($question!=$new_question) {
        drupal_goto("/node/".$this->nid."/take/".$new_question->nid);
      }
      $to_be_rendered['progress']['#markup'] = theme(
        'chem21_quiz_app_quiz_progress', array(
          'quiz' => $this,
          'question' => $question
        ));
      return drupal_render($to_be_rendered);
  }

  public function set_question_weight($vid, $weight) {
    $sql = 'UPDATE {quiz_node_relationship} SET weight = :weight WHERE parent_vid = :quiz_vid AND child_vid = :question_vid';
    $res = db_query($sql, array(':weight'=>$weight, ':quiz_vid'=>$this->vid, ':question_vid'=>$vid))->execute();
    return $res;
  }

  public function reorder_questions($question_orders) {
    $questions = array();
    foreach($question_orders as $qId=>$weight) {
      $question = node_load($qId);
      $question->weight = $weight;
      if(!$question->state) {
        $question->state=1;
      }
      $questions[] = $question;      
    }
    $quiz = $this->node();
    try {
      quiz_set_questions($quiz, $questions);
    } catch(Exception $e) {
      return services_error("Services ".$e->getMessage());
    }
    return $questions;
  }
}