
require 'tree_sitter/core'

module TreeSitter
  class Token
    def to_s
      "#<#{self.class}: (#{byte_range.inspect})>"
    end

    def inspect
      "#<#{self.class}: #{text.inspect} (#{byte_range.inspect})>"
    end
  end
end