<?php
/**
 * @file
 * Provide Drush integration for release building and dependency building.
 */
require_once('git_make.subtree.inc');

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
   // project[example][git-subtree][merge][squash] = TRUE (default)
   // project[example][git-subtree][merge][squash] = FALSE
   // project[example][git-subtree][message] = 'Some message'

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

// drush_print(print_r($info));

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
    else if ($project['download']['type'] != 'git') {
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

/*
drush_print(print('$release:' . "\n"));
drush_print(print_r($release));
drush_print(print_r('$project:' . "\n"));
drush_print(print_r($project));
// */

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

  // "Core is built in place, rather than using make-process."
  if (isset($projects['core'])) {
    git_make_get_subtree('core', $info, $projects['core']['drupal']);
// drush_print(print_r($projects['core']['drupal']));
    // TODO
    // If there are patches download and apply patches.
  }

  // Process all projects.
  if (isset($projects['contrib'])) {
    foreach ($projects['contrib'] as $project) {
      // TODO Consider using drush_backend for something here, like make_projects()?

      git_make_get_subtree($type, $info, $project);
      // TODO
      // If there are patches download and apply patches.
    }
  }
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
// */

/**
 * @param $project
 *  Array. $project
 *
 * @return $url
 *  String. URL of git repo.
 */
function git_make_download_url($project) {
  $download = $project['download'];
  $name = $project['name'];

  // If no download URL specified, assume anonymous clone from git.drupal.org.
  $url = isset($download['url']) ? $download['url'] : "http://git.drupal.org/project/$name.git";

  return $url;
}

// TODO remove this...
function _git_make_stuff_from_make_download_git($name, $download, $download_location) {
  // If no working-copy download URL specified, assume it is the same.
  $download['wc_url'] = isset($download['wc_url']) ? $download['wc_url'] : $download['url'];

  // If not a working copy, and if --no-cache has not been explicitly
  // declared, create a new git reference cache of the remote repository,
  // or update the existing cache to fetch recent changes.
  // @see package_handler_download_project()
  $cache = !$wc && !drush_get_option('no-cache', FALSE);
  if ($cache && ($git_cache = drush_directory_cache('git'))) {
    $project_cache = $git_cache . '/' . $name . '-' . md5($download['url']);
    // Set up a new cache, if it doesn't exist.
    if (!file_exists($project_cache)) {
      $command = 'git clone --mirror';
      if (drush_get_context('DRUSH_VERBOSE')) {
        $command .= ' --verbose --progress';
      }
      $command .= ' %s %s';
      drush_shell_cd_and_exec($git_cache, $command, $download['url'], $project_cache);
    }
    else {
      // Update the --mirror clone.
      drush_shell_cd_and_exec($project_cache, 'git remote update');
    }
    $git_cache = $project_cache;
  }

  // Use working-copy download URL if --working-copy specified.
  $url = $wc ? $download['wc_url'] : $download['url'];

  $tmp_location = drush_tempdir() . '/' . basename($download_location);

  $command = 'git clone %s %s';
  if (drush_get_context('DRUSH_VERBOSE')) {
    $command .= ' --verbose --progress';
  }
  if ($cache) {
    $command .= ' --reference ' . drush_escapeshellarg($git_cache);
  }

  // Before we can checkout anything, we need to clone the repository.
  if (!drush_shell_exec($command, $url, $tmp_location)) {
    make_error('DOWNLOAD_ERROR', dt('Unable to clone @project from @url.', array('@project' => $name, '@url' => $url)));
    return FALSE;
  }

  drush_log(dt('@project cloned from @url.', array('@project' => $name, '@url' => $url)), 'ok');

  // Get the current directory (so we can move back later).
  $cwd = getcwd();
  // Change into the working copy of the cloned repo.
  chdir($tmp_location);

  // We want to use the most specific target possible, so first try a refspec.
  if (!empty($download['refspec'])) {
    if (drush_shell_exec("git fetch %s %s", $url, $download['refspec'])) {
      drush_log(dt("Fetched refspec !refspec.", array('!refspec' => $download['refspec'])), 'ok');

      if (drush_shell_exec("git checkout FETCH_HEAD")) {
        drush_log(dt("Checked out FETCH_HEAD."), 'info');
      }
    }
    else {
      make_error('DOWNLOAD_ERROR', dt("Unable to fetch the refspec @refspec from @project.", array('@refspec' => $download['refspec'], '@project' => $name)));
    }
  }

  // If there wasn't a refspec, try a tag.
  elseif (!empty($download['tag'])) {
    // @TODO: change checkout to refs path.
    if (drush_shell_exec("git checkout %s", 'refs/tags/' . $download['tag'])) {
      drush_log(dt("Checked out tag @tag.", array('@tag' => $download['tag'])), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt("Unable to check out tag @tag.", array('@tag' => $download['tag'])));
    }
  }

  // If there wasn't a tag, try a specific revision hash.
  elseif (!empty($download['revision'])) {
    if (drush_shell_exec("git checkout %s", $download['revision'])) {
      drush_log(dt("Checked out revision @revision.", array('@revision' => $download['revision'])), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt("Unable to checkout revision @revision", array('@revision' => $download['revision'])));
    }
  }

  // If not, see if we at least have a branch.
  elseif (!empty($download['branch'])) {
    if (drush_shell_exec("git checkout %s", $download['branch'])) {
      drush_log(dt("Checked out branch @branch.", array('@branch' => $download['branch'])), 'ok');
    }
    elseif (drush_shell_exec("git checkout -b %s %s", $download['branch'], 'origin/' . $download['branch'])) {
      drush_log(dt('Checked out branch origin/@branch.', array('@branch' => $download['branch'])), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt('Unable to check out branch @branch.', array('@branch' => $download['branch'])));
    }
  }

  if (!empty($download['submodule'])) {
    $command = 'git submodule update';
    foreach ($download['submodule'] as $option) {
      $command .= ' --%s';
    }
    if (call_user_func_array('drush_shell_exec', array_merge(array($command), $download['submodule']))) {
      drush_log(dt('Initialized registered submodules.'), 'ok');
    }
    else {
      make_error('DOWNLOAD_ERROR', dt('Unable to initialize submodules.'));
    }
  }

  // Move back to last current directory (first line).
  chdir($cwd);

  // Move the directory into the final resting location.
  drush_copy_dir($tmp_location, $download_location, TRUE);

  // Remove .git/ directory if working-copy flag was not specified.
  if (!$wc && file_exists($download_location . '/.git')) {
    drush_register_file_for_deletion($download_location . '/.git');
  }

  return dirname($tmp_location);
}

/**
 * Pull project like this:
 *
 *  $ git subtree pull \
 *      --prefix=path/to/project \
 *      --message  TODO make message option
 *      --squash   TODO make squash option
 *      http://example.com/something.git 6.x-1.x
 *
 * @param $project
 *  Array.
 *
 * @return
 *  Boolean.
 */
function git_make_subtree_pull($project) {

  // Get subtree prefix.
  // @todo Nothing sets $contrib_destination yet. Add this.
  $contrib_destination = (isset($contrib_destination)) ? $contrib_destination : 'sites/all/modules';
  $prefix = $contrib_destination . $project['subdir'];

}

/**
 * Check to see if repo exists.
 *
 * @param $repo
 *  String. /path/to/local/repo
 *
 * @return
 *  Boolean
 */
function git_make_local_repo_exists($repo) {
  drush_log(sprintf('Checking for %s', $repo));

  $is_directory = is_dir($repo);
  $is_repo = is_dir("$repo/.git");

  if (!$is_directory && !$is_repo) {
    drush_log(sprintf('Repo does not exist: %s', $repo));
    return FALSE;  
  } 

  drush_log(sprintf('Repo exists: %s', $repo));

  return TRUE;
}

/**
 * Get a local copy of the project's git repo.
 *
 * @param $project
 *  Array
 *
 * @return $repo
 *  String /path/to/repo.
 */
function git_make_get_local_repo($project) {
  $name = $project['name'];
  $url = git_make_download_url($project);

  // Check for local copy of project repo. If it doesn't exist
  // put it here: .drush/repos/$name. If it does exist, try to
  // update it.
  //
  // TODO Add option for user to designate other location.
  // TODO Add option in make file to specify remote(s).
//drush_print(print_r($_ENV));
  $home = $_ENV['HOME'];
  $repos = "$home/.drush-repos";
  if (!is_dir($repos)) {
    $mode = 0777;
    $recursive = TRUE;
    mkdir($repos, $mode, $recursive);
  }
  $repo = sprintf("%s/%s", $repos, $name);
  $use_local = drush_get_option('local');
  $repo_exists = git_make_local_repo_exists($repo);

  if ($repo_exists && $use_local) {
    // Use local checkout of repo as-is. Do nothing here.
    drush_print(sprintf("Using local repo: %s\n", $repo));
  }
  else if ($repo_exists) {
    // Repo exists. Update existing local repo.
    drush_print(sprintf("Checking %s for updates to %s\n", $url, $repo));
    $command = sprintf('cd %s; git pull', $repo);
    _git_make_exec($command);
  }
  else {
    // Repo doesn't exist. Create repo.
    drush_print(sprintf('Cloning %s to %s...', $url, $repo));
    $command = sprintf('cd %s; git clone %s %s', $repos, $url, $name);
    _git_make_exec($command);
    // TODO Add some error handling. E.g. if clone fails.
  }

  // Checkout branch that's going to be pulled. Since we're not cloning from
  // bare repos, we need a local copy of whatever branch we're going to use.
  // This branch doesn't actually have to be checked out while we're pulling.
  // This just seems like the easiest way to handle all scenarios.
  $branch = git_make_get_branch($project);
  $command = sprintf('cd %s; git checkout %s;', $repo, $branch);
  _git_make_exec($command);
 
  return $repo;
}

/**
 * Get commit IDs for project version.
 *
 * @param $core
 *  String. Version of Drupal. e.g. 7.x
 *
 * @param $project
 *  Array.
 *
 * @param $repo
 *  String. /path/to/local/repo
 *
 * @return $id
 *  String. Git commit ID.
 */
function git_make_get_id($core, $project, $repo = FALSE) {
  $id = '';

  $version = (isset($project['version'])) ? $project['version'] : FALSE;
  $tag = (isset($project['download']['tag'])) ? $project['download']['tag'] : FALSE;
  $revision = (isset($project['download']['revision'])) ? $project['download']['revision'] : FALSE;
  $branch = (isset($project['download']['branch'])) ? $project['download']['branch'] : FALSE;
  $type = (isset($project['download']['type'])) ? $project['download']['type'] : FALSE;

  // Get path to local repo if we don't already know it.
  if (!$repo) {
    $repo = git_make_get_local_repo($project);
  }

  // If make file designates a git commit ID, that's all we need.
  if ($type == 'git' && $revision) {
    $id = $revision;
  }
  // Get the commit ID based on the tag specified in the make file.
  else if ($tag) {
    $id = git_make_get_id_from_tag($repo, $tag);
  }
  // Get the commit ID based on the release version specified in
  // the make file.
  else if ($version) {
    // If the project is following Drupal tagging conventions we should
    // be able to figure out the tag we're looking for.
    $tag = ($project['name'] == 'drupal') ? $version : sprintf('%s-%s', $core, $version);
    $id = git_make_get_id_from_tag($repo, $tag);
  }
  // No version is specified in the make file, but a branch is specified,
  // use that.
  else if ($branch) {
    // Find out the commit ID at the tip of $branch.
    $id = git_make_get_id_from_branch($repo, $branch);

     // Add a note to the
    // commit mesage that this is where we're pulling in $branch.
    // TODO    
  }
  // Try to get $id based on recommended version on drupal.org.
  /*
  else if ($version = git_make_get_recommended_version()) {
    // TODO
  }
  // */
  // The make file doesn't specify a version or branch and drupal.org
  // doesn't have a recommended version either. Use the tip of the
  // checked out local branch and hope for the best.
  /*
  else {
   // TODO 
  }
  // */
  
  return $id;
}

/**
 * @param $repo
 *  String. /path/to/local/repo
 *
 * @param $tag
 *  String. Git tag.
 *
 * @return $id
 *  String. Git commit ID (SHA1) corresponding to tag.
 */
function git_make_get_id_from_tag($repo, $tag) {
  $cd = "cd $repo";
  $get_id = "git rev-parse $tag";
  $command = "$cd; $get_id";
  $id = _git_make_exec($command);
  return trim($id);
}

/**
 * @param $repo
 *  String. /path/to/local/repo
 *
 * @param $tag
 *  String. Git tag.
 *
 * @return $id
 *  String. Git commit ID (SHA1) corresponding to tag.
 */
function git_make_get_id_from_branch($repo, $branch) {
  $cd = "cd $repo";
  $checkout_branch = "git checkout $branch";
  $get_id = "git rev-parse HEAD";
  $command = "$cd; $checkout_branch; $get_id;" ;
  $id = _git_make_exec($command);
  return trim($id);
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
function git_make_get_subtree($type, $info, $project) {
  // Determine subtree repo "prefix".
  $prefix = git_make_prefix($type, $project);
  $git_subtree = new GitSubtree($prefix);

  // TODO
  // Recursion(?).

  // Get path to local copy of the project's git repo. Clone or update if needed.
  $git_subtree->repo = git_make_get_local_repo($project);

  // Get commit ID for project version.
  $git_subtree->commit_id = git_make_get_id($info['core'], $project, $repo);


  // CONTINUE HERE
  //  - Something funny is going on with recursion. New docroot in themes.
  //  - Why don't all modules get added?

  if (!is_dir($prefix)) {
    // If the project doesn't exist in the repo yet, add it.
    // First, figure out what branch to use.
    $git_subtree->branch = git_make_get_branch($project);

    $command = $git_subtree->add();
    _git_make_exec($command);
  }
  if (is_dir($prefix)) {
    // TODO Add error handling. E.g. Path can exist but not be a subtree if someone
    //   ran drush make by itself or added the project manually.
    // git subtree pull to update repo.
      // WORK IN PROGRESS git_make_subtree_pull($project);
  }

  // git subree merge to specified release.
  // git subtree merge $tag
    // git make_git_subtree_merge();

}

/**
 * Build prefix path to pass to git subtree.
 *
 * This will be used as the --prefix option like this:
 *   git subtree add --prefix=docroot/sites/all/modules <repo> <branch>
 *
 * Note: You HAVE to build from the top-level directory with git-make.
 * Assuming Drupal docroot is always a subtree of some parent repo.
 *
 * @return
 *   String. git subtree --prefix value.
 */
function git_make_prefix($type, $project) {
  // E.g. $prefix = docroot
  $build_path = $project['build_path'];
  $prefix = $build_path;

  // Build contrib path.
  if ($type == 'contrib') {
    if ($project['type'] != 'profile') {
      // E.g. $prefix = docroot/sites/all
      $prefix .= '/' . $project['contrib_destination'];
    }
    // E.g. $prefix = docroot/sites/all/modules
    // Or $prefix = docroot/profiles
    //   TODO Decide how to handle 'libraries' type here...
    $prefix .= '/' . $project['type'] . 's';
    if ($subdir = $project['subdir']) {
      // E.g. $prefix = docroot/sites/all/modules/contrib
      $prefix .= '/' . $subdir;
    }

    if ($directory_name = $project['directory_name']) {
      // E.g. $prefix = docroot/sites/all/modules/contrib/directory_name
      $prefix .= '/' . $directory_name;
    }
    else {
      $prefix .= '/' . $project['name'];
    }
  }

  return $prefix;
}

/**
 * @param $project
 *  Array.
 *
 * @return
 *  String. Git branch.
 */
function git_make_get_branch($project) {
  $core = $project['core'];

  if ($project['name'] == 'drupal') {
    $branch = $core;
  }
  else if (isset($project['download']['branch'])) {
    $branch = $project['download']['branch'];
  }
  else if (isset($project['version'])) {
    // Turn 1.15 into 1.x
    $project_parts = explode('.', $project['version']);
    $project_version = $project_parts[0] . '.x';
    // Branch!...
    $branch = sprintf('%s-%s', $core, $project_version);
  }
  else {
    $branch = 'master';
  }

  return $branch;
}

/**
 * Execute all shell commands.
 */
function _git_make_exec($command) {
  // TODO use drush_shell_exec here instead?
  drush_log("Running:\n\t$command");
  return shell_exec($command);
}
