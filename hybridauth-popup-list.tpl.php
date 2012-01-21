<h3>Sign in using your account with:</h3>
<ul id="<?php print $widget_id; ?>" class="<?php print $classes; ?>">
<?php foreach ($providers as $provider_id => $icon) { ?>
  <li><?php print $icon; ?></li>
<?php } ?>
</ul>