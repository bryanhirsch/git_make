<?php
/**
 * @file git_make.subtree.inc
 */

/**
 * This class builds and returns git subtree commands to be executed by git_make.
 */
class GitSubtree {
  private $prefix;
  private $repo;
  private $commit_id;
  private $message;
  private $squash;
  private $branch;
  private $root;

  public function __construct($prefix,
                               $repo = '',
                               $commit_id = '',
                               $message = '',
                               $squash = TRUE,
                               $branch = '',
                               $root = '') {

    $this->setPrefix($prefix);
    $this->setRepo($repo);
    $this->setCommit_id($commit_id);
    $this->setMessage($message);
    $this->setSquash($squash);
    $this->setBranch($branch);
    $this->setRoot($root);

  }

  /**
   * gets ========================================================================
   */
  public function getPrefix() { if ($this->prefix) { return $this->prefix; } else { return ''; }}
  public function getRepo() { if ($this->repo) { return $this->repo; } else { return ''; }}
  public function getCommit_id() { if ($this->commit_id) { return $this->commit_id; } else { return ''; }}
  public function getMessage() { if ($this->message) { return $this->message; } else { return ''; }}
  public function getSquash() { if ($this->squash) { return $this->squash; } else { return ''; }}
  public function getBranch() { if ($this->branch) { return $this->branch; } else { return ''; }}
  public function getRoot() { if ($this->root) { return $this->root; } else { return ''; }}

  /**
   * sets ========================================================================
   */
  public function setPrefix($prefix) { $this->prefix = $prefix; }
  public function setRepo($repo) { $this->repo = $repo; }
  public function setCommit_id($commit_id) { $this->commit_id = $commit_id; }
  public function setMessage($message) { $this->message = $message; }
  public function setSquash($squash) { $this->squash = $squash; }
  public function setBranch($branch) { $this->branch = $branch; }
  public function setRoot($root) { $this->root = $root; }

  /**
   * @param string $command
   *  (Optional) git-subtree command being executed.
   *
   * @return string
   *  Custom commit message.
   */
  public function message($command = FALSE) {
    if ($command) {
      if ($message = $this->getMessage()) {
        $message = sprintf("%s\n\tgit subtree %s", $message, $command);
      }
      else {
        $message = "git subtree $command";
      }
      $this->setMessage($message);
    }

   return ($this->getMessage()) ? sprintf('--message="%s"', $this->message) : '';
  }

  /**
   * @return string
   *  Squash flag.
   */
  public function squash() {
    return ($this->getSquash()) ? '--squash' : '';
  }

  /**
   * @return string
   *  Prefix flag.
   */
  public function prefix() {
    return sprintf('--prefix="%s"', $this->getPrefix());
  }

  /**
   * git subtree add
   */
  public function add() {
    $message = $this->message('add');
    $squash = $this->squash();
    $prefix = $this->prefix();
    $repo = $this->getRepo();
    $branch = $this->getBranch();

    $statement = sprintf('git subtree add %s %s %s %s %s', $message, $squash, $prefix, $repo, $branch);
    return $statement;
  }

  /**
   * Execute git subtree pull.
   */
  public function pull() {
    $message = $this->message('pull');
    $squash = $this->squash();
    $prefix = $this->prefix();
    $repo = $this->getRepo();
    $branch = $this->getBranch();

    $statement = sprintf('git subtree pull %s %s %s %s %s', $message, $squash, $prefix, $repo, $branch);
    return $statement;
  }

  /**
   * Execute git subtree merge.
   */
  public function merge() {
    $message = $this->message('merge');
    $squash = $this->squash();
    $prefix = $this->prefix();
    $commit = $this->getCommit_id();

    $statement = sprintf('git subtree merge %s %s %s %s', $message, $squash, $prefix, $commit);
    return $statement;
  }

  // function push() {}
  // function split() {}


}
