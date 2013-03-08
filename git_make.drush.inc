<?php
/**
 * @file
 * Provide Drush integration for release building and dependency building.
 */
require_once('git_make.subtree.inc');
require_once('git_make.gitmake.inc');

/**
 * Implements hook_drush_help().
 */
function git_make_drush_help($section) {
  switch ($section) {
    case 'drush:git-make':
      return dt('TODO add git-make help stuff here.');
  }
}

/**
 * Implements hook_drush_command().
 */
function git_make_drush_command() {
  $items = array();

  $items['git-make'] = array(
    'description' => '',
    'callback' => 'drush_git_make',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap.
    'arguments' => array(
      'path/to/something-build.make' => 'Build file.',
      'path/to/destination' => '(optional) Build site here.',
    ),
    'options' => array(
      'local' => 'Use local ("cached") copy of repos only.',
      'print-only' => 'Print commands, do not execute them.',
      'home' => 'Pass a HOME directory, if your $_ENV variable isnt set.',
    ),
    'examples' => array(
      'drush git-make build-petition.make docroot' => '',
      'drush git-make build-petition.make docroot --local' => '',
    ),
    'aliases' => array('gm'),
  );

  return $items;
}

/**
 * @param $makefile
 *  String. path/to/makefile
 *
 * @param $build_path
 *  String. The path where the final build will be placed.
 */
function drush_git_make($makefile, $build_path = 'docroot') {
  $info = make_parse_info_file($makefile);

  // TODO Make an option for building site like this, with all
  // prefixes pointing to project/* rather than storing everything
  // inside docroot and getting tangled up with nested projects.
   // projects/drupal
   // projects/x
   // projects/y
   // projects/z
   // docroot/                            -> ../projects/drupal
   // docroot/profiles/x                  -> ../../projects/x
   // docroot/sites/all/modules/contrib/y -> ../../../../../projects/y
   // docroot/sites/default/modules       -> ../../../../sites/default/modules
   // docroot/sites/example.com           -> ../../../sites/example.com
   // sites/default/modules/custom/p      -> ../../../../projects/p
  // TODO store these patches to core in git_make to make this ^^ possible
   // - rm sites/ modules/ profiles/
   // - [mkdir sites/ modules/ profiles/]
   // - apply patch to create sites/default, profiles/standard, modules/README

 // TODO Support merge options in .make:
   // project[example][download][type] = 'git-subtree'
   // project[example][download][squash] = TRUE
   // project[example][download][message] = 'Some message'

  /* TODO @see drush_make
  // Support making just a portion of a make file.
  $include_only = array(
    'projects' => array_filter(drush_get_option_list('projects')),
    'libraries' => array_filter(drush_get_option_list('libraries')),
  );
  $info = make_prune_info_file($info, $include_only);
  // */

  if ($info === FALSE || ($info = make_validate_info_file($info)) === FALSE) {
    return FALSE;
  }

  //if (make_projects(FALSE, drush_get_option('contrib-destination', 'sites/all'), $info, $build_path)) {
  // TODO ^^ This is what the call looks like in drush_make(). Update to do this.
  if (git_make_make_projects(FALSE, drush_get_option('contrib-destination', 'sites/all'), $info, $build_path)) {
    //make_libraries(drush_get_option('contrib-destination', 'sites/all'), $info, $build_path);
    git_make_make_libraries(NULL, $info, $build_path);

    /*
    if (drush_get_option('prepare-install')) {
      make_prepare_install($build_path);
    }
    // */
  }

  return $info;
}

/**
 * Call this instead of running Drush core's make_projects().
 *
 * Use same signature as Drush make_projects:
 * @param $recursion              TODO hard-coded FALSE. Add support for recursion.
 *                                     check for .make file. Then kick of new build"
                                       inside this project.
 * @param $contrib_destination    TODO test different contrib destinations.
 * @param $info
 * @param $build_path
 *
 * @todo Submit patch to Drush make to break the big long make_projects() function
 * into smaller parts that can be reused here.
 */
function git_make_make_projects($recursion = FALSE, $contrib_destination, $info, $build_path = 'docroot') {

  drush_include_engine('release_info', 'updatexml');

  // TODO
  // Check for .git and docroot. Must run from top-level directory.

  // TODO parameterize "docroot".

  $projects = array();

  $translations = array();
  if (isset($info['translations'])) {
    $translations = $info['translations'];
  }

  foreach ($info['projects'] as $key => $project) {
    /*
    // TODO Remove this? This should be unnecesary for Git checkouts since commit IDs are SHA1s. Leave code here for now, in case anything blows up without it. Remove later. Maybe we'll still need this for patches?
    $md5 = '';
    if (isset($project['md5'])) {
      $md5 = $project['md5'];
    }
    // */

    // Merge the known data onto the project info.
    $project += array(
      'name'                => $key,
      'core'                => $info['core'],
      'translations'        => $translations,
      'build_path'          => $build_path,
      'contrib_destination' => $contrib_destination,
      'version'             => '',
      'location'            => drush_get_option('make-update-default-url', RELEASE_INFO_DEFAULT_URL),
      'subdir'              => '',
      'directory_name'      => '',
    );

    if (empty($project['download']['type'])) {
      // No download type is specified in make file. The project lives
      // on drupal.org in a Git repo.
      $project['download']['type'] = 'git';
    }
    else if ($project['download']['type'] != 'git' && $project['download']['type'] != 'git-subtree') {
      // If download is specifiec as something other than git, this isn't
      // going to work.
      $type = $project['download']['type'];
      $name = $project['name'];
      $message = "Sorry. Your make file specifies $type as the download "
               . "type for the project: $name. You need to use a Git "
               . "repo to use git-make.";
      drush_print($message);
    }

    // If the 'revision' parameter is passed at the top level, this is
    // short-hand for download revision.
    if (isset($project['revision']) && !isset($project['download'])) {
      $project['download']['revision'] = $project['revision'];
    }

    if (!isset($project['l10n_url']) && ($project['location'] == RELEASE_INFO_DEFAULT_URL)) {
      $project['l10n_url'] = MAKE_DEFAULT_L10N_SERVER;
    }

    // For convenience: define $request to be compatible with release_info
    // engine.
    // TODO: refactor to enforce 'make' to internally work with release_info
    // keys.
    $request = make_prepare_request($project);

    if ($project['location'] != RELEASE_INFO_DEFAULT_URL && !isset($project['type'])) {
      // Set the cache option based on our '--no-cache' option.
      $cache_before = drush_get_option('cache');
      if (!drush_get_option('no-cache', FALSE)) {
        drush_set_option('cache', TRUE);
      }
      $project_type = release_info_check_project($request, 'core');
      // Restore the previous '--cache' option value.
      drush_set_option('cache', $cache_before);
      $project['download_type'] = ($project_type ? 'core' : 'contrib');
    }
    elseif (!empty($project['type'])) {
      $project['download_type'] = ($project['type'] == 'core' ? 'core' : 'contrib');
    }
    else {
      $project['download_type'] = ($project['name'] == 'drupal' ? 'core' : 'contrib');
    }
    $projects[$project['download_type']][$project['name']] = $project;
  }

  $cores = !empty($projects['core']) ? count($projects['core']) : 0;

  if (drush_get_option('no-core')) {
    unset($projects['core']);
  }
  elseif ($cores == 0 && !$recursion) {
    drush_set_error(dt('No core project specified.'));
    return FALSE;
  }
  elseif ($cores == 1 && $recursion) {
    unset($projects['core']);
  }
  elseif ($cores > 1) {
    drush_set_error(dt('More than one core project specified.'));
    return FALSE;
  }

  foreach ($projects as $type => $type_projects) {
    foreach ($type_projects as $project) {
      if (make_project_needs_release_info($project)) {
        // For convenience: define $request to be compatible with release_info
        // engine.
        // TODO: refactor to enforce 'make' to internally work with release_info
        // keys.
        $request = make_prepare_request($project, $type);
        // Set the cache option based on our '--no-cache' option.
        $cache_before = drush_get_option('cache');
        if (!drush_get_option('no-cache', FALSE)) {
          drush_set_option('cache', TRUE);
        }
        $release = release_info_fetch($request);
        // Restore the previous '--cache' option value.
        drush_set_option('cache', $cache_before);
        // Translate release_info key for project_type to drush make.
        $project['type'] = $request['project_type'];


      }

      /*
      if (!empty($md5)) {
        $project['download']['md5'] = $md5;
      }
      if ($ignore_checksums) {
        unset($project['download']['md5']);
      }
      // @todo Remove ^^? Do we need this for patches?
      //  */

      $projects[($project['type'] == 'core' ? 'core' : 'contrib')][$project['name']] = $project;
    }
  }

  $git_make = new GitMake($info, $contrib_destination);

  // "Core is built in place, rather than using make-process."
  if (isset($projects['core'])) {
    // If download type is 'git-subtree', get the subtree.
    if ($git_make->downloadTypeIsSubtree($projects['core']['drupal'])) {
      git_make_get_subtree('core', $info, $projects['core']['drupal']);
    }
    // If download type is not 'git-subtree', just checkout and commit.
    else {
      $git_make->checkoutIndex('core', $info, $projects['core']['drupal']);
    }

    // TODO PATCH.
    if (!empty($projects['core']['drupal']['patches'])) {
      print_r($projects['core']['drupal']);
    }

    // TODO
    // If there are patches download and apply patches.

    // Make symlink to project inside docroot.
    $path = $git_make->getProjectPath('core', $projects['core']['drupal']);
    $command = $git_make->addSymlink($path, $projects['core']['drupal']['name'], TRUE);
    // Commit symlinks.
    $command .= $git_make->commitSymlinks(TRUE);

    _git_make_exec($command);
  }
  // Process all projects.
  if (isset($projects['contrib'])) {
    foreach ($projects['contrib'] as $project) {
      // TODO Consider using drush_backend for something here, like make_projects()?

      // If download type is 'git-subtree', get the subtree.
      if ($git_make->downloadTypeIsSubtree($project)) {
        git_make_get_subtree($type, $info, $project);
      }
      // If download type is not 'git-subtree', just checkout and commit.
      else {
        $git_make->checkoutIndex($type, $info, $project);
      }

      // TODO
      // If there are patches download and apply patches.
      if (!empty($project['patches'])) {
        print_r($project);
      }

      // Make symlink to project inside docroot.
      $path = $git_make->getProjectPath($type, $project);
      $command = $git_make->addSymlink($path, $project['name']);
      _git_make_exec($command);
    }
    // Commit all project symlinks.
    $command = $git_make->commitSymlinks();
    _git_make_exec($command);
  }

  // Garbage collection.
  $command = 'git gc';
  _git_make_exec($command);

  return TRUE;
  // */
}

/**
 * Call this instead of Drush core's make_libraries().
 *
 * Use same signature as Drush make_libraries:
 * @param $contrib_destination    TODO hard-coded NULL
 * @param $info * @param $build_path */
function git_make_make_libraries($contrib_destination = NULL, $info, $build_path) {
  // TODO
}

/**
 * @param $type
 *  String. core, contrib.
 *
 * @param $info
 *  Array. Parsed make file.
 *
 * @param $project
 *  Array. The project for which we're getting a subtree.
 *
 * TODO Extend DrushMakeProject class to build out this subtree functionality.
 *  Move this logic in there.
 */
// TODO Move this to GitMake::getSubtree().
function git_make_get_subtree($type, $info, $project) {
  // Determine subtree repo "prefix".
  $prefix = GitMake::getPrefix($project['name']);

  // Get path to local copy of the project's git repo. Clone or update if needed.
  $repo = GitMake::getLocalRepo($project);

  // Get commit ID for project version.
  $commit_id = GitMake::getId($info['core'], $project, $repo);

  // Figure out what branch to use.
  $branch = GitMake::getBranch($project);

  // Set commit message.
  $message = sprintf('drush git-make executed: %s', date('Y-m-d, H:i', time()));

  // TODO Add a drush option for --squash TRUE.
  // TODO Why isn't this bringing in full history?
  $squash = FALSE;

  // Top-level directory in git repo.
  $root = getcwd();

  $git_subtree = new GitSubtree($prefix, $repo, $commit_id, $message, $squash,
                                $branch, $root);

  // TODO
  // Recursion(?). Recursion probably shouldn't be an option for git-make.

  if (!is_dir($prefix)) {
    // If the project doesn't exist in the repo yet, add it.
    $command = $git_subtree->add();
    _git_make_exec($command);

    // Garbage collection.
    $command = 'git gc';
    _git_make_exec($command);
  }
  else if (is_dir($prefix)) {
    // TODO Add error handling. E.g. Path can exist but not be a subtree if someone
    //   ran drush make by itself or added the project manually.
    $command = $git_subtree->pull();
    _git_make_exec($command);

    // Garbage collection.
    $command = 'git gc';
    _git_make_exec($command);
  }

  // Set a new commit message, for subtree merge commits.
  $tag = GitMake::getTag($project);
  $message = sprintf('drush git-make executed: %s', date('Y-m-d, H:i', time()));
  $message .= sprintf("\n\ttag: %s", $tag);
  $git_subtree->setMessage($message);
  $command = $git_subtree->merge();
  _git_make_exec($command);
  drush_log("Merged project " . $project['name'] . " (version $tag)");

  // Add project info to info file.
  // @todo Probably don't want to add info to subtree projects. Otherwise this
  //   will get committed and pushed up to d.o, which will be annoying.
  //   Should there be a drush option for people who want to add info here
  //   for some reason?...
  /*
  $revision = ($tag) ? $tag : $commit_id;
  $commands = array();
  $commands[] = git_make_append_project_info($revision, $project['name'], $root, $prefix);
  $commands[] = sprintf('git add %s/%s.info; git commit -m "%s"',
    $prefix, $project['name'], 'Added project info. By drush git-make.'
  );
  foreach ($commands as $command) {
    _git_make_exec($command);
  }
  // */


  // Do Git garbage collection, otherwise this repo can become pretty slow.
  // @todo Add an option to ignore garbage collection.
  // @todo Should we manage garbage collection in ~/.drush-repos/* ?
  $command = 'git gc';
  _git_make_exec($command);

}

/**
 * Execute all shell commands.
 *
 * @param boolean $ignore_print_only
 *  Ignore 'print-only' command. This should only be used for shell_exec commands
 *  that do NOT change anything. E.g. The git rev-parse command needs to be
 *  executed to get commit IDs and return working commands when print-only runs.
 */
function _git_make_exec($command, $ignore_print_only = FALSE) {
  if (drush_get_option('print-only') && !$ignore_print_only) {
    return drush_print($command);
  }
  else {

    // TODO Add error handling. E.g. detect 'error' and 'fatal' like this...
    /*
    error: pathspec '7.x-.x' did not match any file(s) known to git.
    fatal: Couldn't find remote ref 7.x-.x
    fatal: The remote end hung up unexpectedly
    // */

    // TODO use drush_shell_exec here instead?
    drush_log("Running: $command");
    return shell_exec($command);
  }
}
