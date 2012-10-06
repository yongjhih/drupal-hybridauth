(function ($) {

Drupal.behaviors.hybridauth_vtabs_SettingsSummary = {};
Drupal.behaviors.hybridauth_vtabs_SettingsSummary.attach = function(context, settings) {
  // Make sure this behavior is processed only if drupalSetSummary is defined.
  if (typeof jQuery.fn.drupalSetSummary == 'undefined') {
    return;
  }
  
  $('fieldset#edit-fset-providers', context).drupalSetSummary(function(context) {
    var vals = [];
    
    $('label', context).each(function (index, Element) {
      var label_for = $(this).attr('for');
      if ($('#' + label_for).is(':checked')) {
        vals.push($.trim($(this).text()));
      }
    });
    
    return vals.join(', ');
  });
  
  $('fieldset#edit-fset-widget', context).drupalSetSummary(function(context) {
    var vals = [];
    
    var value = $('input#edit-hybridauth-widget-title', context).attr('value');
    var label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-widget-title"]', context).text()) + '</span>';
    if (value) {
      vals.push(label + ': ' + value);
    }
    else {
      vals.push(label + ': ' + 'None');
    }
    
    var widget_type;
    label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-widget-type"]', context).text()) + '</span>';
    var list = [];
    $('div#edit-hybridauth-widget-type', context).find('label').each(function(index, Element) {
      var label_for = $(this).attr('for');
      if ($('#' + label_for).is(':checked')) {
        list.push($.trim($(this).text()));
        widget_type = $('#' + label_for).val();
      }
    });
    vals.push(label + ': ' + list.join(', '));
    
    if (widget_type == 'link') {
      value = $('input#edit-hybridauth-widget-link-text', context).attr('value');
      label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-widget-link-text"]', context).text()) + '</span>';
      if (value) {
        vals.push(label + ': ' + value);
      }
      else {
        vals.push(label + ': ' + 'None');
      }
    }
    
    if (widget_type == 'link' || widget_type == 'button') {
      value = $('input#edit-hybridauth-widget-link-title', context).attr('value');
      label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-widget-link-title"]', context).text()) + '</span>';
      if (value) {
        vals.push(label + ': ' + value);
      }
      else {
        vals.push(label + ': ' + 'None');
      }
    }
    
    label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-widget-icon-pack"]', context).text()) + '</span>';
    value = $('select#edit-hybridauth-widget-icon-pack', context).find('option:selected').text();
    vals.push(label + ': ' + value);
    
    label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-widget-weight"]', context).text()) + '</span>';
    value = $('select#edit-hybridauth-widget-weight', context).find('option:selected').text();
    vals.push(label + ': ' + value);
    
    return vals.join('<br />');
  });
  
  $('fieldset#edit-fset-account', context).drupalSetSummary(function(context) {
    var vals = [];
    
    $('label', context).each(function (index, Element) {
      var label_for = $(this).attr('for');
      if ($('#' + label_for).is(':checked')) {
        vals.push($.trim($(this).text()));
      }
      /*var label, value;
      if (label_for == 'edit-hybridauth-username' || label_for == 'edit-hybridauth-display-name') {
        label = '<span style="font-weight:bold;">' + $(this).text() + '</span>';
        value = $('#' + label_for).val();
        vals.push(label + ': ' + value);
      }*/
    });
    
    return vals.join('<br />');
  });
  
  $('fieldset#edit-fset-other', context).drupalSetSummary(function(context) {
    var vals = [];
    
    var value = $('input#edit-hybridauth-destination', context).attr('value');
    var label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-destination"]', context).text()) + '</span>';
    if (value) {
      vals.push(label + ': ' + value);
    }
    else {
      vals.push(label + ': ' + 'return to the same page');
    }
    
    label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-forms"]', context).text()) + '</span>';
    var list = [];
    $('div#edit-hybridauth-forms', context).find('label').each(function(index, Element) {
      var label_for = $(this).attr('for');
      if ($('#' + label_for).is(':checked')) {
        list.push($.trim($(this).text()));
      }
    });
    vals.push(label + ': ' + list.join(', '));
    
    label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-duplicate-emails"]', context).text()) + '</span>';
    list = [];
    $('div#edit-hybridauth-duplicate-emails', context).find('label').each(function(index, Element) {
      var label_for = $(this).attr('for');
      if ($('#' + label_for).is(':checked')) {
        list.push($.trim($(this).text()));
      }
    });
    vals.push(label + ': ' + list.join(', '));
    
    label = '<span style="font-weight:bold;">' + $.trim($('label[for="edit-hybridauth-debug"]', context).text()) + '</span>';
    if ($('input#edit-hybridauth-debug', context).is(':checked')) {
      vals.push(label + ': ' + 'Enabled');
    }
    else {
      vals.push(label + ': ' + 'Disabled');
    }
    
    return vals.join('<br />');
  });
};

})(jQuery);
