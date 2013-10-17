(function ($) {

  Drupal.behaviors.hybridauth_onclick = {};
  Drupal.behaviors.hybridauth_onclick.attach = function(context, settings) {
    $('.hybridauth-onclick-current', context).bind('click', function(index, Element) {
      $(this).parents('.hybridauth-widget').after('<div>' + Drupal.t('Contacting') + ' ' + this.title + '...</div>');
    });
    $('.hybridauth-onclick-popup', context).bind('click', function(index, Element) {
      var width = $(this).attr('data-hybridauth-width'), height = $(this).attr('data-hybridauth-height');
      var popup_window = window.open(
        this.href,
        'hybridauth',
        'location=no,menubar=no,resizable=yes,scrollbars=yes,status=no,titlebar=yes,toolbar=no,channelmode=yes,fullscreen=yes,width=' + width + ',height=' + height
      );
      popup_window.focus();
      return false;
    });
  };

})(jQuery);
